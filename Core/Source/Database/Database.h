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

    enum class InsertMilestoneResult
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

    struct MilestoneInfo
    {
        int Id = 0;
        int ProjectId = 0;
        std::string Name;
        std::string StartDate; // MM-DD-YYYY
        std::string EndDate;   // MM-DD-YYYY
        float ProgressPercentage = 0.0f;
        std::string Status;    // "not started" | "in progress" | "completed"
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
        static UpdateProjectResult UpdateProject(int projectId, const std::string& projectName, const std::string& description, const std::string& userName);

        // Deletes the project and its memberships. Only succeeds if userName is a leader.
        static DeleteProjectResult DeleteProject(int projectId, const std::string& userName);

        // Creates a milestone. Only succeeds if userName is a leader of the project.
        // Dates are expected as MM-DD-YYYY (PostgreSQL DateStyle MDY accepts this).
        static InsertMilestoneResult InsertMilestone(int projectId, const std::string& milestoneName, const std::string& startDate, const std::string& endDate,
            float progressPercentage, const std::string& status, const std::string& userName, int& outMilestoneId);

        // All milestones for a project, ordered by start date then id.
        // Returns false on connection/query failure; true with outMilestones possibly empty.
        static bool GetMilestonesForProject(int projectId, std::vector<MilestoneInfo>& outMilestones);

        // Removes memberName from the project. Only succeeds if actorUserName is a leader,
        // the target is a member, and the actor is not removing themselves.
        static RemoveMemberResult RemoveMember(int projectId, const std::string& memberName,
            const std::string& actorUserName);
    };

}