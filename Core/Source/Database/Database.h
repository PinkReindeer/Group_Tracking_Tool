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

    enum class UpdateMilestoneResult
    {
        Success,
        ProjectNotFound,
        MilestoneNotFound,
        Forbidden, // caller is not a leader of the project
        UserNotFound,
        Error
    };

    enum class DeleteMilestoneResult
    {
        Success,
        ProjectNotFound,
        MilestoneNotFound,
        Forbidden, // caller is not a leader of the project
        UserNotFound,
        Error
    };

    enum class InsertTaskResult
    {
        Success,
        ProjectNotFound,
        MilestoneNotFound,
        MemberNotFound,  // assigned member is not in this project
        InvalidDependency, // dependency missing, wrong project, self, or cycle
        Forbidden,       // caller is not a leader of the project
        UserNotFound,
        Error
    };

    enum class UpdateTaskResult
    {
        Success,
        ProjectNotFound,
        TaskNotFound,
        MilestoneNotFound,
        MemberNotFound,
        InvalidDependency, // dependency missing, wrong project, self, or cycle
        Forbidden,
        UserNotFound,
        Error
    };

    enum class DeleteTaskResult
    {
        Success,
        ProjectNotFound,
        TaskNotFound,
        Forbidden,
        UserNotFound,
        Error
    };

    enum class AcceptTaskResult
    {
        Success,
        ProjectNotFound,
        TaskNotFound,
        Forbidden,       // caller is not the assigned member
        InvalidStatus,   // task is not pending
        DependenciesBlocked, // prerequisite tasks are not all done
        UserNotFound,
        Error
    };

    enum class SubmitTaskResult
    {
        Success,
        ProjectNotFound,
        TaskNotFound,
        Forbidden,       // caller is not the assigned member
        InvalidStatus,   // task is not in progress
        DependenciesBlocked, // prerequisite tasks are not all done
        UserNotFound,
        Error
    };

    enum class ReviewTaskResult
    {
        Success,
        ProjectNotFound,
        TaskNotFound,
        Forbidden,       // caller is not a leader
        InvalidStatus,   // task is not under review
        DependenciesBlocked, // cannot approve until prerequisites are done
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
        int TotalTasks = 0;    // tasks under this milestone
        int DoneTasks = 0;     // tasks with status "done"
    };

    struct MemberInfo
    {
        int MembershipId = 0; // projectmember.membershipid (used when assigning tasks)
        std::string Name;
        std::string JoinDate;
        std::string Role;
    };

    // One prerequisite from taskdependency (taskid depends on DependsOnTaskId).
    struct TaskDependencyInfo
    {
        int DependsOnTaskId = 0;
        std::string DependsOnTaskName;
        std::string DependsOnTaskStatus; // same values as TaskInfo::Status
        bool IsDone = false;             // true when status is "done"
    };

    struct TaskInfo
    {
        int Id = 0;
        int MilestoneId = 0;
        std::string MilestoneName;
        int AssignedMembershipId = 0; // 0 = unassigned
        std::string AssignedMemberName; // empty when unassigned
        std::string Name;
        std::string Description;
        float EstimatedHours = 0.0f;
        std::string Priority;   // "low" | "medium" | "high" | "urgent"
        std::string Deadline;   // MM-DD-YYYY
        std::string Status;     // "backlog" | "pending" | "in progress" | "under review" | "done"
        std::string ReviewComment;
        // Prerequisites: each must be done before this task can be accepted / submitted / approved.
        std::vector<TaskDependencyInfo> Dependencies;
        // True when Dependencies is empty or every prerequisite has status "done".
        bool DependenciesSatisfied = true;
    };

    struct DeliverableInfo
    {
        int LogId = 0;
        int TaskId = 0;
        std::string ExecutionNotes;
        std::string FilePath;
        std::string CodeSnippet;
        std::string SubmissionTime; // display: YYYY-MM-DD HH:MM:SS
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
        // Fills TotalTasks / DoneTasks and derives ProgressPercentage from task completion.
        // Returns false on connection/query failure; true with outMilestones possibly empty.
        static bool GetMilestonesForProject(int projectId, std::vector<MilestoneInfo>& outMilestones);

        // Updates name and dates. Only succeeds if userName is a leader and the milestone belongs to the project.
        // progressPercentage / status are written as provided (caller typically recomputes from tasks/dates).
        static UpdateMilestoneResult UpdateMilestone(int projectId, int milestoneId,
            const std::string& milestoneName, const std::string& startDate, const std::string& endDate,
            float progressPercentage, const std::string& status, const std::string& userName);

        // Deletes a milestone and its tasks. Only succeeds if userName is a leader.
        static DeleteMilestoneResult DeleteMilestone(int projectId, int milestoneId, const std::string& userName);

        // Creates a task under a milestone. Only succeeds if userName is a leader of the project.
        // assignedMembershipId may be 0 for unassigned. deadline is MM-DD-YYYY (empty allowed).
        // dependsOnTaskIds lists prerequisites (same project); empty means no dependencies.
        static InsertTaskResult InsertTask(int projectId, int milestoneId, int assignedMembershipId,
            const std::string& taskName, const std::string& description, float estimatedHours,
            const std::string& priority, const std::string& deadline, const std::string& status,
            const std::vector<int>& dependsOnTaskIds, const std::string& userName, int& outTaskId);

        // All tasks for a project (via milestone), ordered by milestone then task id.
        // Also loads taskdependency rows into each TaskInfo::Dependencies.
        // Returns false on connection/query failure; true with outTasks possibly empty.
        static bool GetTasksForProject(int projectId, std::vector<TaskInfo>& outTasks);

        // Updates a task. Only succeeds if userName is a leader and the task belongs to the project.
        // Replaces the full dependency set with dependsOnTaskIds (empty clears all).
        static UpdateTaskResult UpdateTask(int projectId, int taskId, int milestoneId, int assignedMembershipId,
            const std::string& taskName, const std::string& description, float estimatedHours,
            const std::string& priority, const std::string& deadline, const std::string& status,
            const std::vector<int>& dependsOnTaskIds, const std::string& userName);

        // Deletes a task. Only succeeds if userName is a leader and the task belongs to the project.
        static DeleteTaskResult DeleteTask(int projectId, int taskId, const std::string& userName);

        // Assigned member accepts a pending task → status becomes "in progress".
        static AcceptTaskResult AcceptTask(int projectId, int taskId, const std::string& userName);

        // Assigned member submits work for an in-progress task:
        // inserts deliverablelog (notes, file path, snippet, submissionTime) and sets status "under review".
        // submissionTime is expected as "YYYY-MM-DD HH:MM:SS".
        static SubmitTaskResult SubmitTask(int projectId, int taskId,
            const std::string& executionNotes, const std::string& filePath,
            const std::string& codeSnippet, const std::string& submissionTime,
            const std::string& userName, int& outLogId);

        // Latest deliverable for a task on this project (by log id). Returns false on DB error;
        // true with outFound=false when no submission exists.
        static bool GetLatestDeliverableForTask(int projectId, int taskId,
            DeliverableInfo& outDeliverable, bool& outFound);

        // Leader reviews an under-review task: stores reviewComment on the task.
        // approved=true → status "done"; approved=false → status "in progress".
        static ReviewTaskResult ReviewTask(int projectId, int taskId, bool approved,
            const std::string& reviewComment, const std::string& userName);

        // Removes memberName from the project. Only succeeds if actorUserName is a leader,
        // the target is a member, and the actor is not removing themselves.
        static RemoveMemberResult RemoveMember(int projectId, const std::string& memberName,
            const std::string& actorUserName);
    };

}