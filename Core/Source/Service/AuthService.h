#pragma once

#include <string>

#include "Database/Database.h"

namespace TrackingTool
{

	class AuthService
	{
	public:
		static bool Register(const std::string& userName, const std::string& password, std::string& outMessage);
	};

}