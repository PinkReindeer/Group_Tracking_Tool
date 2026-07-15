#pragma once

#include <string>
#include <vector>

#include "Database/Database.h"

namespace TrackingTool
{

	class ProjectService
	{
	public:
		// Creates a project for the logged-in user. On success, outCode receives the project code
		// and the projects session cache is invalidated.
		static bool CreateProject(const std::string& name, const std::string& description, std::string& outMessage, std::string& outCode);

		// Joins an existing project by its invite code as role "member".
		// On success, outProjectName receives the project name and the cache is invalidated.
		static bool JoinProject(const std::string& projectCode, std::string& outMessage, std::string& outProjectName);

		// Loads projects linked to the logged-in user through projectmember.
		// Uses a session cache unless forceRefresh is true or the cache is empty/stale.
		// Returns false if not logged in or a database error occurred.
		static bool GetUserProjects(std::vector<ProjectInfo>& outProjects, std::string& outMessage, bool forceRefresh = false);

		// Updates name/description for a project the logged-in user leads.
		static bool UpdateProject(int projectId, const std::string& name, const std::string& description, std::string& outMessage);

		// Permanently deletes a project the logged-in user leads (and its memberships).
		static bool DeleteProject(int projectId, std::string& outMessage);

		// Leader-only: creates a milestone with progress 0% and status derived from dates.
		// startDate / endDate must be MM-DD-YYYY and endDate must be on or after startDate.
		static bool CreateMilestone(int projectId, const std::string& name, const std::string& startDate, const std::string& endDate, std::string& outMessage);

		// Loads milestones for a project. Uses a per-project session cache unless forceRefresh.
		static bool GetProjectMilestones(int projectId, std::vector<MilestoneInfo>& outMilestones, std::string& outMessage, bool forceRefresh = false);

		// Leader-only: creates a task under a milestone.
		// assignedMembershipId 0 = unassigned (status backlog); >0 = pending.
		// priority: low | medium | high | urgent. deadline optional MM-DD-YYYY.
		// dependsOnTaskIds: prerequisite tasks that must be Done before this task can progress.
		static bool CreateTask(int projectId, int milestoneId, int assignedMembershipId, const std::string& name, const std::string& description, float estimatedHours,
			const std::string& priority, const std::string& deadline, const std::vector<int>& dependsOnTaskIds, std::string& outMessage);

		// Leader-only: updates a task. Status is adjusted for backlog/pending when assignment changes;
		// advanced statuses (in progress, under review, done) are preserved.
		// dependsOnTaskIds replaces the full prerequisite set (empty clears all).
		static bool UpdateTask(int projectId, int taskId, int milestoneId, int assignedMembershipId, const std::string& name, const std::string& description, float estimatedHours,
			const std::string& priority, const std::string& deadline, const std::string& currentStatus,
			const std::vector<int>& dependsOnTaskIds, std::string& outMessage);

		// Leader-only: permanently deletes a task belonging to the project.
		static bool DeleteTask(int projectId, int taskId, std::string& outMessage);

		// Assigned member accepts a pending task → "in progress".
		static bool AcceptTask(int projectId, int taskId, std::string& outMessage);

		// Assigned member submits an in-progress task with deliverable fields.
		// Computes submission timestamp, stores deliverablelog, sets status "under review".
		// executionNotes, filePath, and codeSnippet are required.
		static bool SubmitTask(int projectId, int taskId,
			const std::string& executionNotes, const std::string& filePath,
			const std::string& codeSnippet, std::string& outMessage);

		// Leader reviews an under-review task. reviewComment is required.
		// approved=true → done; approved=false → in progress (member resubmits).
		static bool ReviewTask(int projectId, int taskId, bool approved,
			const std::string& reviewComment, std::string& outMessage);

		// Latest deliverable submission for a task (for the review UI).
		static bool GetLatestDeliverable(int projectId, int taskId,
			DeliverableInfo& outDeliverable, bool& outFound, std::string& outMessage);

		// Loads tasks for a project. Uses a per-project session cache unless forceRefresh.
		static bool GetProjectTasks(int projectId, std::vector<TaskInfo>& outTasks, std::string& outMessage, bool forceRefresh = false);

		// Removes a member from a project. Only the project leader may do this,
		// and the leader cannot remove themselves.
		static bool RemoveMember(int projectId, const std::string& memberName, std::string& outMessage);

		// Drops the in-memory projects list. Call on logout or after mutations that
		// are not followed by an immediate force-refresh.
		static void InvalidateProjectsCache();

		// Drops cached milestones (all projects, or a single projectId if > 0).
		static void InvalidateMilestonesCache(int projectId = 0);

		// Drops cached tasks (all projects, or a single projectId if > 0).
		static void InvalidateTasksCache(int projectId = 0);

		// Session-selected project (e.g. opened from the dashboard grid).
		static void SetActiveProject(const ProjectInfo& project);
		static void ClearActiveProject();
		// Returns true and fills outProject when a project is selected for this session.
		// Prefer TryGetActiveProject() on hot paths — this copies string fields.
		static bool GetActiveProject(ProjectInfo& outProject);
		// Zero-copy access to the session-cached active project, or nullptr if none.
		static const ProjectInfo* TryGetActiveProject();
		static bool HasActiveProject();

	private:
		static std::vector<ProjectInfo> s_CachedProjects;
		static bool s_HasCache;
		static ProjectInfo s_ActiveProject;
		static bool s_HasActiveProject;

		static std::vector<MilestoneInfo> s_CachedMilestones;
		static int s_CachedMilestonesProjectId;
		static bool s_HasMilestonesCache;

		static std::vector<TaskInfo> s_CachedTasks;
		static int s_CachedTasksProjectId;
		static bool s_HasTasksCache;
	};

}
