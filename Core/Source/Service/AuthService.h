#pragma once

#include <string>

#include "Database/Database.h"

namespace TrackingTool
{

	class AuthService
	{
	public:
		static bool Register(const std::string& userName, const std::string& password, const std::string& confirmPassword, std::string& outMessage);
		static bool Login(const std::string& userName, const std::string& password, std::string& outMessage);
		static void Logout();
		
		static std::string GetLoggedInUser();
		static bool IsLoggedIn();
		
	private:
		static std::string s_LoggedInUser;
	};

}