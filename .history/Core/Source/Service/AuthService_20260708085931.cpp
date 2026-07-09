#include "AuthService.h"

#include <bcrypt/BCrypt.hpp>
#include <cstring>

namespace TrackingTool
{

	bool AuthService::Register(const std::string& userName, const std::string& password, const std::string& confpassword, std::string& outMessage)
	{
		if (userName.empty() || password.empty())
		{
			outMessage = "Username or password cannot be empty.";
			return false;
		}
		else if(strcmp(password, confpassword) != 0)
		{
			outMessage = "Passwords do not match.";
			return false;
		}

		if (Database::UserExists(userName))
		{
			outMessage = "User already exists.";
			return false;
		}

		std::string hashedPassword = BCrypt::generateHash(password);

		if (Database::InsertUser(userName, hashedPassword))
		{
			outMessage = "Registration successful.";
			return true;
		}
		else
		{
			outMessage = "Failed to register user due to database error.";
			return false;
		}
	}

}