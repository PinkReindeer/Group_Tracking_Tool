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
		
		// Returns a reference to the cached username — no allocation. Do not store
		// the reference across Logout() (it is cleared there).
		static const std::string& GetLoggedInUser();
		static bool IsLoggedIn();
		
	private:
		static std::string s_LoggedInUser;
	};

}