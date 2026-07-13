#include "ProjectService.h"

#include "Database/Database.h"
#include "Service/AuthService.h"
#include "Utils/Log.h"
#include "Utils/ProjectUtils.h"
#include "Utils/TimeUtils.h"

namespace TrackingTool
{

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

	bool ProjectService::GetUserProjects(std::vector<ProjectInfo>& outProjects, std::string& outMessage)
	{
		outProjects.clear();
		outMessage.clear();

		if (!AuthService::IsLoggedIn())
		{
			outMessage = "You must be logged in to view projects.";
			Log::Error("ProjectService::GetUserProjects: no user is currently logged in.");
			return false;
		}

		const std::string userName = AuthService::GetLoggedInUser();
		if (!Database::GetProjectsForUser(userName, outProjects))
		{
			outMessage = "Failed to load projects from the database.";
			return false;
		}

		outMessage = "Projects loaded.";
		return true;
	}

}
