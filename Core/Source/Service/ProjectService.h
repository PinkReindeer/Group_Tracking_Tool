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

		// Removes a member from a project. Only the project leader may do this,
		// and the leader cannot remove themselves.
		static bool RemoveMember(int projectId, const std::string& memberName, std::string& outMessage);

		// Drops the in-memory projects list. Call on logout or after mutations that
		// are not followed by an immediate force-refresh.
		static void InvalidateProjectsCache();

		// Session-selected project (e.g. opened from the dashboard grid).
		static void SetActiveProject(const ProjectInfo& project);
		static void ClearActiveProject();
		// Returns true and fills outProject when a project is selected for this session.
		static bool GetActiveProject(ProjectInfo& outProject);
		static bool HasActiveProject();

	private:
		static std::vector<ProjectInfo> s_CachedProjects;
		static bool s_HasCache;
		static ProjectInfo s_ActiveProject;
		static bool s_HasActiveProject;
	};

}
