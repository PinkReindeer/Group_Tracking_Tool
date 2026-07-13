#include "AuthService.h"

#include <bcrypt/BCrypt.hpp>
#include <cstring>

namespace TrackingTool
{

	std::string AuthService::s_LoggedInUser = "";

	bool AuthService::Register(const std::string& userName, const std::string& password, const std::string& confirmPassword, std::string& outMessage)
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
		else if(password != confirmPassword)
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
				s_LoggedInUser = userName;
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
			outMessage = std::string("Invalid password format in database.") + e.what();
			return false;
		}
	}

	void AuthService::Logout()
	{
		s_LoggedInUser.clear();
	}

	const std::string& AuthService::GetLoggedInUser()
	{
		return s_LoggedInUser;
	}

	bool AuthService::IsLoggedIn()
	{
		return !s_LoggedInUser.empty();
	}

}