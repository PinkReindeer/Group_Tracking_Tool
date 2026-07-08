#pragma once

#include <string>

namespace TrackingTool
{

    struct Database
    {
        static void Init();

        static std::string GetDatabaseURI();

        static bool InsertUser(const std::string& userName, const std::string& hashedPassword);
        static bool UserExists(const std::string& userName);
        static std::string GetUserPasswordHash(const std::string& userName);
    };

}