#pragma once

#include <string>
#include <vector>

namespace TrackingTool
{

    enum class InsertProjectResult
    {
        Success,
        DuplicateCode,
        UserNotFound,
        Error
    };

    enum class JoinProjectResult
    {
        Success,
        ProjectNotFound,
        AlreadyMember,
        UserNotFound,
        Error
    };

    struct ProjectInfo
    {
        int Id = 0;
        std::string Name;
        std::string Code;
        std::string Description;
        std::string CreatedDate;
        std::string Role; // e.g. "leader", "member"
    };

    struct Database
    {
        static void Init();

        static std::string GetDatabaseURI();

        static bool InsertUser(const std::string& userName, const std::string& hashedPassword);
        static bool UserExists(const std::string& userName);
        static std::string GetUserPasswordHash(const std::string& userName);

        static InsertProjectResult InsertProject(const std::string& projectName, const std::string& projectCode, const std::string& description, 
            const std::string& createdDate, const std::string& creatorUserName);

        // Adds userName to the project matching projectCode as role "member".
        // On success, outProjectName receives the project display name.
        static JoinProjectResult JoinProjectByCode(const std::string& projectCode, const std::string& userName, const std::string& joinDate, std::string& outProjectName);

        // Projects the user belongs to via projectmember (junction table).
        // Returns false on connection/query failure; true with outProjects possibly empty.
        static bool GetProjectsForUser(const std::string& userName, std::vector<ProjectInfo>& outProjects);
    };

}
