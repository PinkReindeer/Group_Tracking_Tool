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

	void ProjectService::InvalidateProjectsCache()
	{
		s_CachedProjects.clear();
		s_HasCache = false;
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

}
