#include "ProjectService.h"

#include "Database/Database.h"
#include "Service/AuthService.h"
#include "Utils/Log.h"
#include "Utils/ProjectUtils.h"
#include "Utils/TimeUtils.h"

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
	}

	std::vector<ProjectInfo> ProjectService::s_CachedProjects;
	bool ProjectService::s_HasCache = false;
	ProjectInfo ProjectService::s_ActiveProject;
	bool ProjectService::s_HasActiveProject = false;

	std::vector<MilestoneInfo> ProjectService::s_CachedMilestones;
	int ProjectService::s_CachedMilestonesProjectId = 0;
	bool ProjectService::s_HasMilestonesCache = false;

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

	void ProjectService::InvalidateProjectsCache()
	{
		s_CachedProjects.clear();
		s_HasCache = false;
	}

	void ProjectService::InvalidateMilestonesCache(int projectId)
	{
		if (projectId > 0 && s_HasMilestonesCache && s_CachedMilestonesProjectId != projectId)
			return;

		s_CachedMilestones.clear();
		s_CachedMilestonesProjectId = 0;
		s_HasMilestonesCache = false;
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

		// Recompute display status from progress + dates so list stays accurate as days pass.
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
