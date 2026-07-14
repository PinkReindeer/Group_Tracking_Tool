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

    enum class UpdateProjectResult
    {
        Success,
        ProjectNotFound,
        Forbidden, // caller is not a leader of the project
        UserNotFound,
        Error
    };

    enum class DeleteProjectResult
    {
        Success,
        ProjectNotFound,
        Forbidden, // caller is not a leader of the project
        UserNotFound,
        Error
    };

    enum class RemoveMemberResult
    {
        Success,
        ProjectNotFound,
        Forbidden,       // actor is not a leader of the project
        CannotRemoveSelf,// leader cannot remove themselves
        MemberNotFound,  // target is not a member of the project
        UserNotFound,    // actor username not found
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

    struct MemberInfo
    {
        std::string Name;
        std::string JoinDate;
        std::string Role;
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

        static bool GetProjectMembers(int projectId, std::vector<MemberInfo>& outMembers);

        // Updates name/description. Only succeeds if userName is a leader of the project.
        static UpdateProjectResult UpdateProject(int projectId, const std::string& projectName,
            const std::string& description, const std::string& userName);

        // Deletes the project and its memberships. Only succeeds if userName is a leader.
        static DeleteProjectResult DeleteProject(int projectId, const std::string& userName);

        // Removes memberName from the project. Only succeeds if actorUserName is a leader,
        // the target is a member, and the actor is not removing themselves.
        static RemoveMemberResult RemoveMember(int projectId, const std::string& memberName,
            const std::string& actorUserName);
    };

}