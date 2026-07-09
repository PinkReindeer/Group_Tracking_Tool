#include "AuthService.h"

#include <bcrypt/BCrypt.hpp>
#include <cstring>

namespace TrackingTool
{

	bool AuthService::Register(const std::string& userName, const std::string& password, const std::string& confpassword, std::string& outMessage)
	{
		outMessage.clear();

		if (userName.empty() || password.empty())
		{
			outMessage = "Username or password cannot be empty.";
			return false;
		}
		else if(password.size() < 8)
		{
			outMessage = "Passwords must be longer than 8 characters!";
			return false;
		}
		else if(password != confpassword)
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

	bool AuthService::Login(const std::string& userName, const std::string& password, std::string& outMessage)
	{
		outMessage.clear();

		if (userName.empty() || password.empty())
		{
			outMessage = "Username or password cannot be empty.";
			return false;
		}

		std::string hashedPassword = Database::GetUserPasswordHash(userName);
		if (hashedPassword.empty())
		{
			outMessage = "User not found or database error.";
			return false;
		}

		try
		{
			if (BCrypt::validatePassword(password, hashedPassword))
			{
				outMessage = "Login successful.";
				return true;
			}
			else
			{
				outMessage = "Incorrect username or password.";
				return false;
			}
		}
		catch (const std::exception& e)
		{
			outMessage = "Invalid password format in database.";
			return false;
		}
	}

}