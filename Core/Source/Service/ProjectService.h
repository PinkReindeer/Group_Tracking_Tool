#pragma once

#include <string>
#include <vector>

#include "Database/Database.h"

namespace TrackingTool
{

	class ProjectService
	{
	public:
		// Creates a project for the logged-in user. On success, outCode receives the project code.
		static bool CreateProject(const std::string& name, const std::string& description,
			std::string& outMessage, std::string& outCode);

		// Loads projects linked to the logged-in user through projectmember.
		// Returns false if not logged in or a database error occurred.
		static bool GetUserProjects(std::vector<ProjectInfo>& outProjects, std::string& outMessage);
	};

}
