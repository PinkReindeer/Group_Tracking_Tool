#include "ProjectService.h"

#include "Database/Database.h"
#include "Service/AuthService.h"
#include "Utils/Log.h"
#include "Utils/ProjectUtils.h"
#include "Utils/TimeUtils.h"

#include <algorithm>
#include <cctype>

namespace TrackingTool
{

	namespace
	{
		// Trim whitespace and uppercase so invite codes match GenerateProjectCode format.
		std::string NormalizeProjectCode(const std::string& raw)
		{
			std::string code;
			code.reserve(raw.size());
			for (unsigned char ch : raw)
			{
				if (std::isspace(ch))
					continue;
				code.push_back(static_cast<char>(std::toupper(ch)));
			}
			return code;
		}

		// Case-insensitive ASCII compare against a literal — no heap allocation.
		bool EqualsIgnoreCase(const std::string& value, const char* literal)
		{
			size_t i = 0;
			for (; i < value.size() && literal[i] != '\0'; ++i)
			{
				const unsigned char a = static_cast<unsigned char>(value[i]);
				const unsigned char b = static_cast<unsigned char>(literal[i]);
				if (std::tolower(a) != std::tolower(b))
					return false;
			}
			return i == value.size() && literal[i] == '\0';
		}

		// Overdue when deadline is before today and the task is not already done.
		bool IsTaskOverdue(const TaskInfo& task, const char* todayMmDdYyyy)
		{
			if (task.Deadline.empty() || EqualsIgnoreCase(task.Status, "done"))
				return false;
			if (!todayMmDdYyyy || !Utils::IsValidMmDdYyyy(task.Deadline.c_str()))
				return false;
			return Utils::CompareMmDdYyyy(task.Deadline.c_str(), todayMmDdYyyy) < 0;
		}
	}

	std::vector<ProjectInfo> ProjectService::s_CachedProjects;
	bool ProjectService::s_HasCache = false;
	ProjectInfo ProjectService::s_ActiveProject;
	bool ProjectService::s_HasActiveProject = false;

	std::vector<MilestoneInfo> ProjectService::s_CachedMilestones;
	int ProjectService::s_CachedMilestonesProjectId = 0;
	bool ProjectService::s_HasMilestonesCache = false;
	int ProjectService::s_MilestonesCacheGeneration = 0;
	int ProjectService::s_TasksCacheGeneration = 0;

	std::vector<TaskInfo> ProjectService::s_CachedTasks;
	int ProjectService::s_CachedTasksProjectId = 0;
	bool ProjectService::s_HasTasksCache = false;

	std::vector<MemberInfo> ProjectService::s_CachedMembers;
	int ProjectService::s_CachedMembersProjectId = 0;
	bool ProjectService::s_HasMembersCache = false;
	int ProjectService::s_MembersCacheGeneration = 0;

	std::vector<ProjectService::TaskNotificationInfo> ProjectService::s_TaskNotifications;
	int ProjectService::s_PendingNotificationCount = 0;
	int ProjectService::s_OverdueNotificationCount = 0;
	bool ProjectService::s_HasTaskNotifications = false;
	bool ProjectService::s_RebuildingTaskNotifications = false;

	namespace
	{
		std::string TrimCopy(const std::string& raw)
		{
			size_t begin = 0;
			while (begin < raw.size() && std::isspace(static_cast<unsigned char>(raw[begin])))
				++begin;
			size_t end = raw.size();
			while (end > begin && std::isspace(static_cast<unsigned char>(raw[end - 1])))
				--end;
			return raw.substr(begin, end - begin);
		}
	}

	void ProjectService::ClearTaskNotifications()
	{
		s_TaskNotifications.clear();
		s_PendingNotificationCount = 0;
		s_OverdueNotificationCount = 0;
		s_HasTaskNotifications = false;
	}

	namespace
	{
		void AppendTaskNotificationsForProject(
			const ProjectInfo& project,
			const std::vector<TaskInfo>& tasks,
			const std::string& userName,
			const char* today,
			std::vector<ProjectService::TaskNotificationInfo>& outItems)
		{
			for (const TaskInfo& task : tasks)
			{
				if (!EqualsIgnoreCase(task.AssignedMemberName, userName.c_str()))
					continue;

				const bool overdue = IsTaskOverdue(task, today);
				const bool pending = EqualsIgnoreCase(task.Status, "pending");

				// Overdue takes priority so a late pending task appears once under Overdue.
				if (overdue)
				{
					ProjectService::TaskNotificationInfo item;
					item.TaskId = task.Id;
					item.ProjectId = project.Id;
					item.ProjectName = project.Name;
					item.TaskName = task.Name;
					item.Deadline = task.Deadline;
					item.IsOverdue = true;
					outItems.push_back(std::move(item));
				}
				else if (pending)
				{
					ProjectService::TaskNotificationInfo item;
					item.TaskId = task.Id;
					item.ProjectId = project.Id;
					item.ProjectName = project.Name;
					item.TaskName = task.Name;
					item.Deadline = task.Deadline;
					item.IsOverdue = false;
					outItems.push_back(std::move(item));
				}
			}
		}

		void SortAndCountTaskNotifications(
			std::vector<ProjectService::TaskNotificationInfo>& items,
			int& outPending,
			int& outOverdue)
		{
			std::stable_sort(items.begin(), items.end(),
				[](const ProjectService::TaskNotificationInfo& a,
					const ProjectService::TaskNotificationInfo& b)
				{
					if (a.IsOverdue != b.IsOverdue)
						return a.IsOverdue && !b.IsOverdue;
					if (a.ProjectName != b.ProjectName)
						return a.ProjectName < b.ProjectName;
					return a.TaskName < b.TaskName;
				});

			outPending = 0;
			outOverdue = 0;
			for (const ProjectService::TaskNotificationInfo& item : items)
			{
				if (item.IsOverdue)
					++outOverdue;
				else
					++outPending;
			}
		}
	}

	void ProjectService::RefreshTaskNotifications(bool forceRefresh, int onlyProjectId)
	{
		if (s_RebuildingTaskNotifications)
			return;

		if (!AuthService::IsLoggedIn())
		{
			ClearTaskNotifications();
			return;
		}

		// Incremental: single project mutation while an inbox already exists.
		// Full rebuild: login, membership change, explicit refresh, or no inbox yet.
		const bool singleProject = (onlyProjectId > 0 && s_HasTaskNotifications);

		s_RebuildingTaskNotifications = true;

		std::string message;
		std::vector<ProjectInfo> projects;
		// Project list rarely changes on task create — prefer cache for single-project path.
		if (!GetUserProjects(projects, message, forceRefresh && !singleProject))
		{
			s_RebuildingTaskNotifications = false;
			return;
		}

		const std::string& userName = AuthService::GetLoggedInUser();
		const char* today = Utils::GetTodayMmDdYyyy();

		if (singleProject)
		{
			// Drop rows for this project, reload only its tasks from DB.
			s_TaskNotifications.erase(
				std::remove_if(s_TaskNotifications.begin(), s_TaskNotifications.end(),
					[onlyProjectId](const TaskNotificationInfo& item)
					{
						return item.ProjectId == onlyProjectId;
					}),
				s_TaskNotifications.end());

			const ProjectInfo* project = nullptr;
			for (const ProjectInfo& p : projects)
			{
				if (p.Id == onlyProjectId)
				{
					project = &p;
					break;
				}
			}

			if (project)
			{
				std::vector<TaskInfo> tasks;
				// Always force this project's tasks after a mutation so the inbox matches the DB.
				if (GetProjectTasks(project->Id, tasks, message, true))
					AppendTaskNotificationsForProject(*project, tasks, userName, today, s_TaskNotifications);
			}

			SortAndCountTaskNotifications(s_TaskNotifications, s_PendingNotificationCount, s_OverdueNotificationCount);
			s_HasTaskNotifications = true;
			s_RebuildingTaskNotifications = false;

			Log::Info("ProjectService::RefreshTaskNotifications: incremental project {} → {} pending, {} overdue.",
				onlyProjectId, s_PendingNotificationCount, s_OverdueNotificationCount);
			return;
		}

		// Full rebuild (login, membership change, explicit refresh).
		ClearTaskNotifications();

		for (const ProjectInfo& project : projects)
		{
			std::vector<TaskInfo> tasks;
			if (!GetProjectTasks(project.Id, tasks, message, forceRefresh))
				continue;
			AppendTaskNotificationsForProject(project, tasks, userName, today, s_TaskNotifications);
		}

		SortAndCountTaskNotifications(s_TaskNotifications, s_PendingNotificationCount, s_OverdueNotificationCount);
		s_HasTaskNotifications = true;
		s_RebuildingTaskNotifications = false;

		Log::Info("ProjectService::RefreshTaskNotifications: full → {} pending, {} overdue for '{}'.",
			s_PendingNotificationCount, s_OverdueNotificationCount, userName);
	}

	const std::vector<ProjectService::TaskNotificationInfo>& ProjectService::GetTaskNotifications()
	{
		return s_TaskNotifications;
	}

	int ProjectService::GetPendingNotificationCount()
	{
		return s_PendingNotificationCount;
	}

	int ProjectService::GetOverdueNotificationCount()
	{
		return s_OverdueNotificationCount;
	}

	bool ProjectService::HasTaskNotifications()
	{
		return s_HasTaskNotifications;
	}

	void ProjectService::InvalidateProjectsCache()
	{
		s_CachedProjects.clear();
		s_HasCache = false;
		// Membership roster may have changed with projects.
		InvalidateMembersCache();
		// Membership list changed — drop inbox, then rebuild if still logged in
		// (join/create/delete project). Login clears the user first so this is a no-op there.
		ClearTaskNotifications();
		if (!s_RebuildingTaskNotifications && AuthService::IsLoggedIn())
			RefreshTaskNotifications(true, 0);
	}

	void ProjectService::InvalidateMilestonesCache(int projectId)
	{
		if (projectId > 0 && s_HasMilestonesCache && s_CachedMilestonesProjectId != projectId)
			return;

		s_CachedMilestones.clear();
		s_CachedMilestonesProjectId = 0;
		s_HasMilestonesCache = false;
		++s_MilestonesCacheGeneration;
	}

	int ProjectService::GetMilestonesCacheGeneration()
	{
		return s_MilestonesCacheGeneration;
	}

	void ProjectService::InvalidateMembersCache(int projectId)
	{
		if (projectId > 0 && s_HasMembersCache && s_CachedMembersProjectId != projectId)
			return;

		s_CachedMembers.clear();
		s_CachedMembersProjectId = 0;
		s_HasMembersCache = false;
		++s_MembersCacheGeneration;
	}

	int ProjectService::GetMembersCacheGeneration()
	{
		return s_MembersCacheGeneration;
	}

	void ProjectService::InvalidateTasksCache(int projectId)
	{
		// Always bump generation so views (TasksView) reload even if the single-slot
		// task cache currently holds a different project.
		++s_TasksCacheGeneration;

		if (!(projectId > 0 && s_HasTasksCache && s_CachedTasksProjectId != projectId))
		{
			s_CachedTasks.clear();
			s_CachedTasksProjectId = 0;
			s_HasTasksCache = false;
		}

		// Rebuild inbox: one project only when possible (create/edit/accept/submit/review).
		// Skip while already rebuilding, and while logged out (login/logout cache wipes).
		if (!s_RebuildingTaskNotifications && AuthService::IsLoggedIn())
			RefreshTaskNotifications(false, projectId);
	}

	int ProjectService::GetTasksCacheGeneration()
	{
		return s_TasksCacheGeneration;
	}

	void ProjectService::SetActiveProject(const ProjectInfo& project)
	{
		s_ActiveProject = project;
		s_HasActiveProject = true;
	}

	void ProjectService::ClearActiveProject()
	{
		s_ActiveProject = ProjectInfo{};
		s_HasActiveProject = false;
	}

	bool ProjectService::GetActiveProject(ProjectInfo& outProject)
	{
		if (!s_HasActiveProject)
			return false;
		outProject = s_ActiveProject;
		return true;
	}

	const ProjectInfo* ProjectService::TryGetActiveProject()
	{
		return s_HasActiveProject ? &s_ActiveProject : nullptr;
	}

	bool ProjectService::HasActiveProject()
	{
		return s_HasActiveProject;
	}

	bool ProjectService::CreateProject(const std::string& name, const std::string& description,
		std::string& outMessage, std::string& outCode)
	{
		outMessage.clear();
		outCode.clear();

		if (name.empty())
		{
			outMessage = "Project name is required.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to create a project.";
			Log::Error("ProjectService::CreateProject: no user is currently logged in.");
			return false;
		}

		const std::string creator = AuthService::GetLoggedInUser();
		const std::string createdDate = Utils::GetDateTime();

		constexpr int kMaxAttempts = 10;
		for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
		{
			const std::string code = Utils::GenerateProjectCode();
			const InsertProjectResult result = Database::InsertProject(name, code, description, createdDate, creator);

			switch (result)
			{
			case InsertProjectResult::Success:
				// New membership/project means the dashboard list is stale.
				InvalidateProjectsCache();
				outCode = code;
				outMessage = "Project created successfully. Code: " + code;
				return true;

			case InsertProjectResult::DuplicateCode:
				Log::Warn("ProjectService::CreateProject: code '{}' already exists. Retrying... ({}/{})",
					code, attempt + 1, kMaxAttempts);
				continue;

			case InsertProjectResult::UserNotFound:
				outMessage = "Logged-in user was not found in the database.";
				return false;

			case InsertProjectResult::Error:
			default:
				outMessage = "Failed to create project due to a database error.";
				return false;
			}
		}

		outMessage = "Could not allocate a unique project code. Please try again.";
		Log::Error("ProjectService::CreateProject: failed to generate a unique code after {} attempts.", kMaxAttempts);
		return false;
	}

	bool ProjectService::JoinProject(const std::string& projectCode, std::string& outMessage,
		std::string& outProjectName)
	{
		outMessage.clear();
		outProjectName.clear();

		const std::string code = NormalizeProjectCode(projectCode);
		if (code.empty())
		{
			outMessage = "Project code is required.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to join a project.";
			Log::Error("ProjectService::JoinProject: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const std::string joinDate = Utils::GetDateTime();
		const JoinProjectResult result = Database::JoinProjectByCode(code, userName, joinDate, outProjectName);

		switch (result)
		{
		case JoinProjectResult::Success:
			InvalidateProjectsCache();
			outMessage = "Joined project \"" + outProjectName + "\" successfully.";
			return true;

		case JoinProjectResult::ProjectNotFound:
			outMessage = "No project found with that code.";
			return false;

		case JoinProjectResult::AlreadyMember:
			outMessage = outProjectName.empty()
				? "You are already a member of this project."
				: "You are already a member of \"" + outProjectName + "\".";
			return false;

		case JoinProjectResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case JoinProjectResult::Error:
		default:
			outMessage = "Failed to join project due to a database error.";
			return false;
		}
	}

	bool ProjectService::GetUserProjects(std::vector<ProjectInfo>& outProjects, std::string& outMessage,
		bool forceRefresh)
	{
		outProjects.clear();
		outMessage.clear();

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to view projects.";
			Log::Error("ProjectService::GetUserProjects: no user is currently logged in.");
			return false;
		}

		if (s_HasCache && !forceRefresh)
		{
			outProjects = s_CachedProjects;
			outMessage = "Projects loaded from cache.";
			return true;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		if (!Database::GetProjectsForUser(userName, outProjects))
		{
			outMessage = "Failed to load projects from the database.";
			return false;
		}

		s_CachedProjects = outProjects;
		s_HasCache = true;
		outMessage = "Projects loaded.";
		return true;
	}

	bool ProjectService::UpdateProject(int projectId, const std::string& name,
		const std::string& description, std::string& outMessage)
	{
		outMessage.clear();

		if (name.empty())
		{
			outMessage = "Project name is required.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to edit a project.";
			Log::Error("ProjectService::UpdateProject: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const UpdateProjectResult result = Database::UpdateProject(projectId, name, description, userName);

		switch (result)
		{
		case UpdateProjectResult::Success:
			InvalidateProjectsCache();
			if (s_HasActiveProject && s_ActiveProject.Id == projectId)
			{
				s_ActiveProject.Name = name;
				s_ActiveProject.Description = description;
			}
			outMessage = "Project updated successfully.";
			return true;

		case UpdateProjectResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case UpdateProjectResult::Forbidden:
			outMessage = "Only the project leader can edit this project.";
			return false;

		case UpdateProjectResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case UpdateProjectResult::Error:
		default:
			outMessage = "Failed to update project due to a database error.";
			return false;
		}
	}

	bool ProjectService::DeleteProject(int projectId, std::string& outMessage)
	{
		outMessage.clear();

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to delete a project.";
			Log::Error("ProjectService::DeleteProject: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const DeleteProjectResult result = Database::DeleteProject(projectId, userName);

		switch (result)
		{
		case DeleteProjectResult::Success:
			InvalidateProjectsCache();
			InvalidateMilestonesCache(projectId);
			InvalidateTasksCache(projectId);
			if (s_HasActiveProject && s_ActiveProject.Id == projectId)
				ClearActiveProject();
			outMessage = "Project deleted successfully.";
			return true;

		case DeleteProjectResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case DeleteProjectResult::Forbidden:
			outMessage = "Only the project leader can delete this project.";
			return false;

		case DeleteProjectResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case DeleteProjectResult::Error:
		default:
			outMessage = "Failed to delete project due to a database error.";
			return false;
		}
	}

	bool ProjectService::CreateMilestone(int projectId, const std::string& name,
		const std::string& startDate, const std::string& endDate, std::string& outMessage)
	{
		outMessage.clear();

		const std::string trimmedName = TrimCopy(name);
		const std::string trimmedStart = TrimCopy(startDate);
		const std::string trimmedEnd = TrimCopy(endDate);

		if (trimmedName.empty())
		{
			outMessage = "Milestone name is required.";
			return false;
		}

		if (!Utils::IsValidMmDdYyyy(trimmedStart))
		{
			outMessage = "Start date must be a valid date in MM-DD-YYYY format.";
			return false;
		}

		if (!Utils::IsValidMmDdYyyy(trimmedEnd))
		{
			outMessage = "End date must be a valid date in MM-DD-YYYY format.";
			return false;
		}

		if (Utils::CompareMmDdYyyy(trimmedEnd, trimmedStart) < 0)
		{
			outMessage = "End date must be on or after the start date.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to create a milestone.";
			Log::Error("ProjectService::CreateMilestone: no user is currently logged in.");
			return false;
		}

		constexpr float kInitialProgress = 0.0f;
		const std::string status = Utils::ComputeMilestoneStatus(kInitialProgress, trimmedStart, trimmedEnd);
		const std::string userName = AuthService::GetLoggedInUser();

		int milestoneId = 0;
		const InsertMilestoneResult result = Database::InsertMilestone(
			projectId, trimmedName, trimmedStart, trimmedEnd,
			kInitialProgress, status, userName, milestoneId);

		switch (result)
		{
		case InsertMilestoneResult::Success:
			InvalidateMilestonesCache(projectId);
			outMessage = "Milestone \"" + trimmedName + "\" created successfully.";
			return true;

		case InsertMilestoneResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case InsertMilestoneResult::Forbidden:
			outMessage = "Only the project leader can create milestones.";
			return false;

		case InsertMilestoneResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case InsertMilestoneResult::Error:
		default:
			outMessage = "Failed to create milestone due to a database error.";
			return false;
		}
	}

	bool ProjectService::GetProjectMilestones(int projectId, std::vector<MilestoneInfo>& outMilestones,
		std::string& outMessage, bool forceRefresh)
	{
		outMilestones.clear();
		outMessage.clear();

		if (projectId <= 0)
		{
			outMessage = "Invalid project.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to view milestones.";
			Log::Error("ProjectService::GetProjectMilestones: no user is currently logged in.");
			return false;
		}

		if (s_HasMilestonesCache && !forceRefresh && s_CachedMilestonesProjectId == projectId)
		{
			outMilestones = s_CachedMilestones;
			outMessage = "Milestones loaded from cache.";
			return true;
		}

		if (!Database::GetMilestonesForProject(projectId, outMilestones))
		{
			outMessage = "Failed to load milestones from the database.";
			return false;
		}

		// Progress comes from task completion (done/total). Status is completed at 100%,
		// otherwise derived from the planned date window.
		for (MilestoneInfo& m : outMilestones)
		{
			m.Status = Utils::ComputeMilestoneStatus(m.ProgressPercentage, m.StartDate, m.EndDate);
		}

		s_CachedMilestones = outMilestones;
		s_CachedMilestonesProjectId = projectId;
		s_HasMilestonesCache = true;
		outMessage = "Milestones loaded.";
		return true;
	}

	bool ProjectService::UpdateMilestone(int projectId, int milestoneId, const std::string& name,
		const std::string& startDate, const std::string& endDate, std::string& outMessage)
	{
		outMessage.clear();

		const std::string trimmedName = TrimCopy(name);
		const std::string trimmedStart = TrimCopy(startDate);
		const std::string trimmedEnd = TrimCopy(endDate);

		if (projectId <= 0 || milestoneId <= 0)
		{
			outMessage = "Invalid milestone.";
			return false;
		}

		if (trimmedName.empty())
		{
			outMessage = "Milestone name is required.";
			return false;
		}

		if (!Utils::IsValidMmDdYyyy(trimmedStart))
		{
			outMessage = "Start date must be a valid date in MM-DD-YYYY format.";
			return false;
		}

		if (!Utils::IsValidMmDdYyyy(trimmedEnd))
		{
			outMessage = "End date must be a valid date in MM-DD-YYYY format.";
			return false;
		}

		if (Utils::CompareMmDdYyyy(trimmedEnd, trimmedStart) < 0)
		{
			outMessage = "End date must be on or after the start date.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to edit a milestone.";
			Log::Error("ProjectService::UpdateMilestone: no user is currently logged in.");
			return false;
		}

		// Preserve current task-based progress when rewriting name/dates.
		float progress = 0.0f;
		{
			std::vector<MilestoneInfo> milestones;
			std::string loadMsg;
			if (GetProjectMilestones(projectId, milestones, loadMsg, false))
			{
				for (const MilestoneInfo& m : milestones)
				{
					if (m.Id == milestoneId)
					{
						progress = m.ProgressPercentage;
						break;
					}
				}
			}
		}

		const std::string status = Utils::ComputeMilestoneStatus(progress, trimmedStart, trimmedEnd);
		const std::string userName = AuthService::GetLoggedInUser();

		const UpdateMilestoneResult result = Database::UpdateMilestone(
			projectId, milestoneId, trimmedName, trimmedStart, trimmedEnd,
			progress, status, userName);

		switch (result)
		{
		case UpdateMilestoneResult::Success:
			InvalidateMilestonesCache(projectId);
			outMessage = "Milestone \"" + trimmedName + "\" updated successfully.";
			return true;

		case UpdateMilestoneResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case UpdateMilestoneResult::MilestoneNotFound:
			outMessage = "Milestone not found.";
			return false;

		case UpdateMilestoneResult::Forbidden:
			outMessage = "Only the project leader can edit milestones.";
			return false;

		case UpdateMilestoneResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case UpdateMilestoneResult::Error:
		default:
			outMessage = "Failed to update milestone due to a database error.";
			return false;
		}
	}

	bool ProjectService::DeleteMilestone(int projectId, int milestoneId, std::string& outMessage)
	{
		outMessage.clear();

		if (projectId <= 0 || milestoneId <= 0)
		{
			outMessage = "Invalid milestone.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to delete a milestone.";
			Log::Error("ProjectService::DeleteMilestone: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const DeleteMilestoneResult result = Database::DeleteMilestone(projectId, milestoneId, userName);

		switch (result)
		{
		case DeleteMilestoneResult::Success:
			InvalidateMilestonesCache(projectId);
			InvalidateTasksCache(projectId);
			outMessage = "Milestone deleted successfully.";
			return true;

		case DeleteMilestoneResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case DeleteMilestoneResult::MilestoneNotFound:
			outMessage = "Milestone not found.";
			return false;

		case DeleteMilestoneResult::Forbidden:
			outMessage = "Only the project leader can delete milestones.";
			return false;

		case DeleteMilestoneResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case DeleteMilestoneResult::Error:
		default:
			outMessage = "Failed to delete milestone due to a database error.";
			return false;
		}
	}

	namespace
	{
		bool IsValidTaskPriority(const std::string& priority)
		{
			return priority == "low" || priority == "medium"
				|| priority == "high" || priority == "urgent";
		}
	}

	bool ProjectService::CreateTask(int projectId, int milestoneId, int assignedMembershipId,
		const std::string& name, const std::string& description, float estimatedHours,
		const std::string& priority, const std::string& deadline,
		const std::vector<int>& dependsOnTaskIds, std::string& outMessage)
	{
		outMessage.clear();

		const std::string trimmedName = TrimCopy(name);
		const std::string trimmedDescription = TrimCopy(description);
		const std::string trimmedPriority = TrimCopy(priority);
		const std::string trimmedDeadline = TrimCopy(deadline);

		if (projectId <= 0)
		{
			outMessage = "Invalid project.";
			return false;
		}

		if (milestoneId <= 0)
		{
			outMessage = "A milestone is required to create a task.";
			return false;
		}

		if (trimmedName.empty())
		{
			outMessage = "Task name is required.";
			return false;
		}

		if (estimatedHours < 0.0f)
		{
			outMessage = "Estimated hours cannot be negative.";
			return false;
		}

		// Normalize priority to lowercase for storage/validation.
		std::string priorityLower;
		priorityLower.reserve(trimmedPriority.size());
		for (unsigned char ch : trimmedPriority)
			priorityLower.push_back(static_cast<char>(std::tolower(ch)));

		if (!IsValidTaskPriority(priorityLower))
		{
			outMessage = "Priority must be low, medium, high, or urgent.";
			return false;
		}

		if (!trimmedDeadline.empty() && !Utils::IsValidMmDdYyyy(trimmedDeadline))
		{
			outMessage = "Deadline must be a valid date in MM-DD-YYYY format.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to create a task.";
			Log::Error("ProjectService::CreateTask: no user is currently logged in.");
			return false;
		}

		// Initial status: backlog when unassigned, pending when a member is chosen.
		const std::string status = (assignedMembershipId > 0) ? "pending" : "backlog";
		const std::string userName = AuthService::GetLoggedInUser();

		int taskId = 0;
		const InsertTaskResult result = Database::InsertTask(
			projectId, milestoneId, assignedMembershipId,
			trimmedName, trimmedDescription, estimatedHours,
			priorityLower, trimmedDeadline, status, dependsOnTaskIds, userName, taskId);

		switch (result)
		{
		case InsertTaskResult::Success:
			InvalidateTasksCache(projectId);
			InvalidateMilestonesCache(projectId); // task totals/progress changed
			outMessage = "Task \"" + trimmedName + "\" created successfully.";
			return true;

		case InsertTaskResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case InsertTaskResult::MilestoneNotFound:
			outMessage = "Selected milestone was not found on this project.";
			return false;

		case InsertTaskResult::MemberNotFound:
			outMessage = "Selected member is not part of this project.";
			return false;

		case InsertTaskResult::InvalidDependency:
			outMessage = "Invalid task dependency (missing task, wrong project, self-dependency, or cycle).";
			return false;

		case InsertTaskResult::Forbidden:
			outMessage = "Only the project leader can create tasks.";
			return false;

		case InsertTaskResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case InsertTaskResult::Error:
		default:
			outMessage = "Failed to create task due to a database error.";
			return false;
		}
	}

	namespace
	{
		// Keep advanced workflow statuses; only bounce backlog <-> pending with assignment.
		std::string ResolveTaskStatusOnEdit(const std::string& currentStatus, int assignedMembershipId)
		{
			const bool assigned = assignedMembershipId > 0;
			if (currentStatus == "in progress" || currentStatus == "under review" || currentStatus == "done")
				return currentStatus;

			return assigned ? "pending" : "backlog";
		}
	}

	bool ProjectService::UpdateTask(int projectId, int taskId, int milestoneId, int assignedMembershipId,
		const std::string& name, const std::string& description, float estimatedHours,
		const std::string& priority, const std::string& deadline, const std::string& currentStatus,
		const std::vector<int>& dependsOnTaskIds, std::string& outMessage)
	{
		outMessage.clear();

		const std::string trimmedName = TrimCopy(name);
		const std::string trimmedDescription = TrimCopy(description);
		const std::string trimmedPriority = TrimCopy(priority);
		const std::string trimmedDeadline = TrimCopy(deadline);

		if (projectId <= 0 || taskId <= 0)
		{
			outMessage = "Invalid task.";
			return false;
		}

		if (milestoneId <= 0)
		{
			outMessage = "A milestone is required.";
			return false;
		}

		if (trimmedName.empty())
		{
			outMessage = "Task name is required.";
			return false;
		}

		if (estimatedHours < 0.0f)
		{
			outMessage = "Estimated hours cannot be negative.";
			return false;
		}

		std::string priorityLower;
		priorityLower.reserve(trimmedPriority.size());
		for (unsigned char ch : trimmedPriority)
			priorityLower.push_back(static_cast<char>(std::tolower(ch)));

		if (!IsValidTaskPriority(priorityLower))
		{
			outMessage = "Priority must be low, medium, high, or urgent.";
			return false;
		}

		if (!trimmedDeadline.empty() && !Utils::IsValidMmDdYyyy(trimmedDeadline))
		{
			outMessage = "Deadline must be a valid date in MM-DD-YYYY format.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to edit a task.";
			Log::Error("ProjectService::UpdateTask: no user is currently logged in.");
			return false;
		}

		const std::string status = ResolveTaskStatusOnEdit(TrimCopy(currentStatus), assignedMembershipId);
		const std::string userName = AuthService::GetLoggedInUser();

		const UpdateTaskResult result = Database::UpdateTask(
			projectId, taskId, milestoneId, assignedMembershipId,
			trimmedName, trimmedDescription, estimatedHours,
			priorityLower, trimmedDeadline, status, dependsOnTaskIds, userName);

		switch (result)
		{
		case UpdateTaskResult::Success:
			InvalidateTasksCache(projectId);
			InvalidateMilestonesCache(projectId); // milestone assignment / progress may change
			outMessage = "Task \"" + trimmedName + "\" updated successfully.";
			return true;

		case UpdateTaskResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case UpdateTaskResult::TaskNotFound:
			outMessage = "Task not found.";
			return false;

		case UpdateTaskResult::MilestoneNotFound:
			outMessage = "Selected milestone was not found on this project.";
			return false;

		case UpdateTaskResult::MemberNotFound:
			outMessage = "Selected member is not part of this project.";
			return false;

		case UpdateTaskResult::InvalidDependency:
			outMessage = "Invalid task dependency (missing task, wrong project, self-dependency, or cycle).";
			return false;

		case UpdateTaskResult::Forbidden:
			outMessage = "Only the project leader can edit tasks.";
			return false;

		case UpdateTaskResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case UpdateTaskResult::Error:
		default:
			outMessage = "Failed to update task due to a database error.";
			return false;
		}
	}

	bool ProjectService::DeleteTask(int projectId, int taskId, std::string& outMessage)
	{
		outMessage.clear();

		if (projectId <= 0 || taskId <= 0)
		{
			outMessage = "Invalid task.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to delete a task.";
			Log::Error("ProjectService::DeleteTask: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const DeleteTaskResult result = Database::DeleteTask(projectId, taskId, userName);

		switch (result)
		{
		case DeleteTaskResult::Success:
			InvalidateTasksCache(projectId);
			InvalidateMilestonesCache(projectId); // task totals/progress changed
			outMessage = "Task deleted successfully.";
			return true;

		case DeleteTaskResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case DeleteTaskResult::TaskNotFound:
			outMessage = "Task not found.";
			return false;

		case DeleteTaskResult::Forbidden:
			outMessage = "Only the project leader can delete tasks.";
			return false;

		case DeleteTaskResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case DeleteTaskResult::Error:
		default:
			outMessage = "Failed to delete task due to a database error.";
			return false;
		}
	}

	bool ProjectService::AcceptTask(int projectId, int taskId, std::string& outMessage)
	{
		outMessage.clear();

		if (projectId <= 0 || taskId <= 0)
		{
			outMessage = "Invalid task.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to accept a task.";
			Log::Error("ProjectService::AcceptTask: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const AcceptTaskResult result = Database::AcceptTask(projectId, taskId, userName);

		switch (result)
		{
		case AcceptTaskResult::Success:
			InvalidateTasksCache(projectId);
			outMessage = "Task accepted. Status is now In Progress.";
			return true;

		case AcceptTaskResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case AcceptTaskResult::TaskNotFound:
			outMessage = "Task not found.";
			return false;

		case AcceptTaskResult::Forbidden:
			outMessage = "Only the assigned member can accept this task.";
			return false;

		case AcceptTaskResult::InvalidStatus:
			outMessage = "Only pending tasks can be accepted.";
			return false;

		case AcceptTaskResult::DependenciesBlocked:
			outMessage = "Cannot accept this task until all prerequisite tasks are Done.";
			return false;

		case AcceptTaskResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case AcceptTaskResult::Error:
		default:
			outMessage = "Failed to accept task due to a database error.";
			return false;
		}
	}

	bool ProjectService::SubmitTask(int projectId, int taskId,
		const std::string& executionNotes, const std::string& filePath,
		const std::string& codeSnippet, std::string& outMessage)
	{
		outMessage.clear();

		const std::string notes = TrimCopy(executionNotes);
		const std::string path = TrimCopy(filePath);
		const std::string snippet = TrimCopy(codeSnippet);

		if (projectId <= 0 || taskId <= 0)
		{
			outMessage = "Invalid task.";
			return false;
		}

		if (notes.empty())
		{
			outMessage = "Execution notes are required.";
			return false;
		}

		if (path.empty())
		{
			outMessage = "File path is required.";
			return false;
		}

		if (snippet.empty())
		{
			outMessage = "Code snippet is required.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to submit a task.";
			Log::Error("ProjectService::SubmitTask: no user is currently logged in.");
			return false;
		}

		// Capture local submission time for deliverablelog.submissiontime.
		const std::string submissionTime = Utils::GetSubmissionTimestamp();
		const std::string userName = AuthService::GetLoggedInUser();

		int logId = 0;
		const SubmitTaskResult result = Database::SubmitTask(
			projectId, taskId, notes, path, snippet, submissionTime, userName, logId);

		switch (result)
		{
		case SubmitTaskResult::Success:
			InvalidateTasksCache(projectId);
			outMessage = "Task submitted for review at " + submissionTime + ".";
			return true;

		case SubmitTaskResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case SubmitTaskResult::TaskNotFound:
			outMessage = "Task not found.";
			return false;

		case SubmitTaskResult::Forbidden:
			outMessage = "Only the assigned member can submit this task.";
			return false;

		case SubmitTaskResult::InvalidStatus:
			outMessage = "Only in-progress tasks can be submitted. Accept the task first.";
			return false;

		case SubmitTaskResult::DependenciesBlocked:
			outMessage = "Cannot submit this task until all prerequisite tasks are Done.";
			return false;

		case SubmitTaskResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case SubmitTaskResult::Error:
		default:
			outMessage = "Failed to submit task due to a database error.";
			return false;
		}
	}

	bool ProjectService::ReviewTask(int projectId, int taskId, bool approved,
		const std::string& reviewComment, std::string& outMessage)
	{
		outMessage.clear();

		const std::string comment = TrimCopy(reviewComment);

		if (projectId <= 0 || taskId <= 0)
		{
			outMessage = "Invalid task.";
			return false;
		}

		if (comment.empty())
		{
			outMessage = "Review comment is required.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to review a task.";
			Log::Error("ProjectService::ReviewTask: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		const ReviewTaskResult result = Database::ReviewTask(
			projectId, taskId, approved, comment, userName);

		switch (result)
		{
		case ReviewTaskResult::Success:
			InvalidateTasksCache(projectId);
			if (approved)
				InvalidateMilestonesCache(projectId); // completion % may hit 100%
			outMessage = approved
				? "Task approved. Status is now Done."
				: "Task rejected. Status is In Progress — member can resubmit.";
			return true;

		case ReviewTaskResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case ReviewTaskResult::TaskNotFound:
			outMessage = "Task not found.";
			return false;

		case ReviewTaskResult::Forbidden:
			outMessage = "Only the project leader can review submissions.";
			return false;

		case ReviewTaskResult::InvalidStatus:
			outMessage = "Only tasks under review can be approved or rejected.";
			return false;

		case ReviewTaskResult::DependenciesBlocked:
			outMessage = "Cannot approve this task until all prerequisite tasks are Done.";
			return false;

		case ReviewTaskResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case ReviewTaskResult::Error:
		default:
			outMessage = "Failed to review task due to a database error.";
			return false;
		}
	}

	bool ProjectService::GetLatestDeliverable(int projectId, int taskId,
		DeliverableInfo& outDeliverable, bool& outFound, std::string& outMessage)
	{
		outDeliverable = DeliverableInfo{};
		outFound = false;
		outMessage.clear();

		if (projectId <= 0 || taskId <= 0)
		{
			outMessage = "Invalid task.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to view submissions.";
			return false;
		}

		if (!Database::GetLatestDeliverableForTask(projectId, taskId, outDeliverable, outFound))
		{
			outMessage = "Failed to load submission from the database.";
			return false;
		}

		outMessage = outFound ? "Submission loaded." : "No submission found.";
		return true;
	}

	bool ProjectService::GetProjectTasks(int projectId, std::vector<TaskInfo>& outTasks,
		std::string& outMessage, bool forceRefresh)
	{
		outTasks.clear();
		outMessage.clear();

		if (projectId <= 0)
		{
			outMessage = "Invalid project.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to view tasks.";
			Log::Error("ProjectService::GetProjectTasks: no user is currently logged in.");
			return false;
		}

		if (s_HasTasksCache && !forceRefresh && s_CachedTasksProjectId == projectId)
		{
			outTasks = s_CachedTasks;
			outMessage = "Tasks loaded from cache.";
			return true;
		}

		if (!Database::GetTasksForProject(projectId, outTasks))
		{
			outMessage = "Failed to load tasks from the database.";
			return false;
		}

		s_CachedTasks = outTasks;
		s_CachedTasksProjectId = projectId;
		s_HasTasksCache = true;
		outMessage = "Tasks loaded.";
		return true;
	}

	bool ProjectService::GetProjectMembers(int projectId, std::vector<MemberInfo>& outMembers,
		std::string& outMessage, bool forceRefresh)
	{
		outMembers.clear();
		outMessage.clear();

		if (projectId <= 0)
		{
			outMessage = "Invalid project.";
			return false;
		}

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to view members.";
			Log::Error("ProjectService::GetProjectMembers: no user is currently logged in.");
			return false;
		}

		if (s_HasMembersCache && !forceRefresh && s_CachedMembersProjectId == projectId)
		{
			outMembers = s_CachedMembers;
			outMessage = "Members loaded from cache.";
			return true;
		}

		if (!Database::GetProjectMembers(projectId, outMembers))
		{
			outMessage = "Failed to load project members from the database.";
			return false;
		}

		s_CachedMembers = outMembers;
		s_CachedMembersProjectId = projectId;
		s_HasMembersCache = true;
		outMessage = "Members loaded.";
		return true;
	}

	bool ProjectService::RemoveMember(int projectId, const std::string& memberName, std::string& outMessage)
	{
		outMessage.clear();

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to remove a member.";
			Log::Error("ProjectService::RemoveMember: no user is currently logged in.");
			return false;
		}

		const std::string actorUserName = AuthService::GetLoggedInUser();
		const RemoveMemberResult result = Database::RemoveMember(projectId, memberName, actorUserName);

		switch (result)
		{
		case RemoveMemberResult::Success:
			// Removed member loses access; refresh project list if they were viewing it.
			InvalidateProjectsCache();
			outMessage = "Removed \"" + memberName + "\" from the project.";
			return true;

		case RemoveMemberResult::ProjectNotFound:
			outMessage = "Project not found.";
			return false;

		case RemoveMemberResult::Forbidden:
			outMessage = "Only the project leader can remove members.";
			return false;

		case RemoveMemberResult::CannotRemoveSelf:
			outMessage = "You cannot remove yourself from the project.";
			return false;

		case RemoveMemberResult::MemberNotFound:
			outMessage = "That user is not a member of this project.";
			return false;

		case RemoveMemberResult::UserNotFound:
			outMessage = "Logged-in user was not found in the database.";
			return false;

		case RemoveMemberResult::Error:
		default:
			outMessage = "Failed to remove member due to a database error.";
			return false;
		}
	}

}
