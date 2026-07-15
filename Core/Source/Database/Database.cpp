#include "Database.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <pqxx/pqxx>

#include "Utils/Log.h"

namespace TrackingTool
{

    static std::filesystem::path FindEnvFile()
    {
        std::filesystem::path dir = std::filesystem::current_path();

        for (int i = 0; i < 10; ++i)
        {
            auto candidate = dir / ".env";
            if (std::filesystem::exists(candidate))
                return candidate;

            auto parent = dir.parent_path();
            if (parent == dir)
                break;
            dir = parent;
        }

        return {};
    }

    static std::unique_ptr<pqxx::connection> s_Connection;

    void Database::Init()
    {
        const std::string uri = GetDatabaseURI();
        if (uri.empty())
        {
            Log::Error("Database::Init: database URI is empty");
            return;
        }

        try
        {
            s_Connection = std::make_unique<pqxx::connection>(uri);
            if (s_Connection->is_open())
            {
                Log::Info("Successfully connected to cloud database securely!");
            }
        }
        catch (const std::exception& e)
        {
            Log::Error("Database::Init: Failed to connect to database: {}", e.what());
        }
    }

    std::string Database::GetDatabaseURI()
    {
        const std::filesystem::path envPath = FindEnvFile();
        if (envPath.empty())
        {
            Log::Error("GetDatabaseURI: .env file not found in any parent directory");
            return {};
        }

        std::ifstream file(envPath);
        if (!file.is_open())
        {
            Log::Error("GetDatabaseURI: failed to open {}", envPath.string());
            return {};
        }

        const std::string key = "DATABASE_URI=";
        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            if (line.starts_with(key))
            {
                std::string value = line.substr(key.size());

                if (!value.empty() && value.back() == '\r')
                    value.pop_back();

                Log::Trace("GetDatabaseURI: loaded URI from {}", envPath.string());
                return value;
            }
        }

        Log::Error("GetDatabaseURI: DATABASE_URI key not found in {}", envPath.string());
        return {};
    }

    bool Database::InsertUser(const std::string& userName, const std::string& hashedPassword)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_InsertUser: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            txn.exec_params("INSERT INTO users (username, password) VALUES ($1, $2)", userName, hashedPassword);
            txn.commit();

            Log::Info("DB_InsertUser: user '{}' registered", userName);
            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_InsertUser: {}", e.what());
            return false;
        }
    }

    bool Database::UserExists(const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_UserExists: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            pqxx::result res = txn.exec_params("SELECT 1 FROM users WHERE username = $1", userName);

            return !res.empty();
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_UserExists: {}", e.what());
            return false;
        }
    }

    std::string Database::GetUserPasswordHash(const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_GetUserPasswordHash: Database is not connected!");
            return "";
        }

        try
        {
            pqxx::work txn(*s_Connection);
            pqxx::result res = txn.exec_params("SELECT password FROM users WHERE username = $1", userName);
            
            if (res.empty())
                return "";
                
            return res[0][0].as<std::string>();
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_GetUserPasswordHash: {}", e.what());
            return "";
        }
    }

    InsertProjectResult Database::InsertProject(const std::string& projectName, const std::string& projectCode, const std::string& description, const std::string& createdDate, const std::string& creatorUserName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_InsertProject: Database is not connected!");
            return InsertProjectResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // Get creator user ID
            pqxx::result userRes = txn.exec_params("SELECT userid FROM users WHERE username = $1", creatorUserName);
            if (userRes.empty())
            {
                Log::Error("DB_InsertProject: Creator user not found.");
                return InsertProjectResult::UserNotFound;
            }
            std::string userId = userRes[0][0].as<std::string>();

            // Insert Project
            pqxx::result projectRes;
            if (description.empty())
            {
                projectRes = txn.exec_params("INSERT INTO project (projectname, projectcode, projectdescription, createddate) VALUES ($1, $2, NULL, $3) RETURNING projectid", projectName, projectCode, createdDate);
            }
            else
            {
                projectRes = txn.exec_params("INSERT INTO project (projectname, projectcode, projectdescription, createddate) VALUES ($1, $2, $3, $4) RETURNING projectid", projectName, projectCode, description, createdDate);
            }

            if (projectRes.empty())
            {
                Log::Error("DB_InsertProject: Failed to retrieve project ID after insertion.");
                return InsertProjectResult::Error;
            }
            int projectId = projectRes[0][0].as<int>();

            // Insert Project Member (creator as leader)
            txn.exec_params("INSERT INTO projectmember (projectid, userid, role, joindate) VALUES ($1, $2, $3, $4)", projectId, userId, "leader", createdDate);

            txn.commit();

            Log::Info("DB_InsertProject: project '{}' created by user '{}'", projectName, creatorUserName);
            return InsertProjectResult::Success;
        }
        catch (const pqxx::sql_error& e)
        {
            if (std::string(e.sqlstate()) == "23505")
            {
                Log::Warn("DB_InsertProject: unique constraint violated for project code '{}'", projectCode);
                return InsertProjectResult::DuplicateCode;
            }

            Log::Error("DB_InsertProject: {}", e.what());
            return InsertProjectResult::Error;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_InsertProject: {}", e.what());
            return InsertProjectResult::Error;
        }
    }

    JoinProjectResult Database::JoinProjectByCode(const std::string& projectCode, const std::string& userName, const std::string& joinDate, std::string& outProjectName)
    {
        outProjectName.clear();

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_JoinProjectByCode: Database is not connected!");
            return JoinProjectResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // Resolve the user first so we fail clearly if the session is stale.
            pqxx::result userRes = txn.exec_params("SELECT userid FROM users WHERE username = $1", userName);
            if (userRes.empty())
            {
                Log::Error("DB_JoinProjectByCode: user '{}' not found.", userName);
                return JoinProjectResult::UserNotFound;
            }
            const std::string userId = userRes[0][0].as<std::string>();

            // Project codes are stored uppercase (see GenerateProjectCode).
            pqxx::result projectRes = txn.exec_params(
                "SELECT projectid, projectname FROM project WHERE projectcode = $1",
                projectCode);
            if (projectRes.empty())
            {
                Log::Warn("DB_JoinProjectByCode: no project with code '{}'", projectCode);
                return JoinProjectResult::ProjectNotFound;
            }

            const int projectId = projectRes[0][0].as<int>();
            outProjectName = projectRes[0][1].as<std::string>();

            // Already a member?
            pqxx::result memberRes = txn.exec_params("SELECT 1 FROM projectmember WHERE projectid = $1 AND userid = $2", projectId, userId);
            if (!memberRes.empty())
            {
                Log::Warn("DB_JoinProjectByCode: user '{}' already in project '{}'", userName, projectCode);
                return JoinProjectResult::AlreadyMember;
            }

            txn.exec_params("INSERT INTO projectmember (projectid, userid, role, joindate) VALUES ($1, $2, $3, $4)", projectId, userId, "member", joinDate);

            txn.commit();

            Log::Info("DB_JoinProjectByCode: user '{}' joined project '{}' ({}) as member", userName, outProjectName, projectCode);
            return JoinProjectResult::Success;
        }
        catch (const pqxx::sql_error& e)
        {
            // Race: unique constraint if another concurrent join inserted the same row.
            if (std::string(e.sqlstate()) == "23505")
            {
                Log::Warn("DB_JoinProjectByCode: unique constraint — already a member (code '{}')", projectCode);
                return JoinProjectResult::AlreadyMember;
            }

            Log::Error("DB_JoinProjectByCode: {}", e.what());
            return JoinProjectResult::Error;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_JoinProjectByCode: {}", e.what());
            return JoinProjectResult::Error;
        }
    }

    bool Database::GetProjectsForUser(const std::string& userName, std::vector<ProjectInfo>& outProjects)
    {
        outProjects.clear();

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_GetProjectsForUser: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // projectmember links users <-> project; return this user's memberships with role.
            pqxx::result res = txn.exec_params(
                "SELECT p.projectid, p.projectname, p.projectcode, p.projectdescription, p.createddate, pm.role "
                "FROM project p "
                "INNER JOIN projectmember pm ON p.projectid = pm.projectid "
                "INNER JOIN users u ON pm.userid = u.userid "
                "WHERE u.username = $1 "
                "ORDER BY p.createddate DESC, p.projectid DESC",
                userName);

            outProjects.reserve(static_cast<size_t>(res.size()));
            for (const auto& row : res)
            {
                ProjectInfo info;
                info.Id = row["projectid"].as<int>();
                info.Name = row["projectname"].as<std::string>();
                info.Code = row["projectcode"].as<std::string>();
                if (!row["projectdescription"].is_null())
                    info.Description = row["projectdescription"].as<std::string>();
                if (!row["createddate"].is_null())
                    info.CreatedDate = row["createddate"].as<std::string>();
                info.Role = row["role"].as<std::string>();
                outProjects.push_back(std::move(info));
            }

            Log::Info("DB_GetProjectsForUser: loaded {} project(s) for '{}'", outProjects.size(), userName);
            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_GetProjectsForUser: {}", e.what());
            outProjects.clear();
            return false;
        }
    }

    namespace
    {
        // Returns empty optional-like pair: first = found user, second = role (empty if not a member).
        // Uses the open transaction. On DB failure throws.
        bool ResolveUserAndRole(pqxx::work& txn, int projectId, const std::string& userName,
            std::string& outUserId, std::string& outRole, bool& outProjectExists)
        {
            outUserId.clear();
            outRole.clear();
            outProjectExists = false;

            pqxx::result userRes = txn.exec_params("SELECT userid FROM users WHERE username = $1", userName);
            if (userRes.empty())
                return false;
            outUserId = userRes[0][0].as<std::string>();

            pqxx::result projectRes = txn.exec_params("SELECT 1 FROM project WHERE projectid = $1", projectId);
            outProjectExists = !projectRes.empty();
            if (!outProjectExists)
                return true;

            pqxx::result roleRes = txn.exec_params(
                "SELECT role FROM projectmember WHERE projectid = $1 AND userid = $2",
                projectId, outUserId);
            if (!roleRes.empty())
                outRole = roleRes[0][0].as<std::string>();
            return true;
        }

        bool IsLeaderRoleString(const std::string& role)
        {
            static constexpr char kLeader[] = "leader";
            if (role.size() != sizeof(kLeader) - 1)
                return false;
            for (size_t i = 0; i < role.size(); ++i)
            {
                const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(role[i])));
                if (c != kLeader[i])
                    return false;
            }
            return true;
        }
    }

    UpdateProjectResult Database::UpdateProject(int projectId, const std::string& projectName,
        const std::string& description, const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_UpdateProject: Database is not connected!");
            return UpdateProjectResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_UpdateProject: user '{}' not found.", userName);
                return UpdateProjectResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_UpdateProject: project id {} not found.", projectId);
                return UpdateProjectResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_UpdateProject: user '{}' is not leader of project {}.", userName, projectId);
                return UpdateProjectResult::Forbidden;
            }

            if (description.empty())
            {
                txn.exec_params(
                    "UPDATE project SET projectname = $1, projectdescription = NULL WHERE projectid = $2",
                    projectName, projectId);
            }
            else
            {
                txn.exec_params(
                    "UPDATE project SET projectname = $1, projectdescription = $2 WHERE projectid = $3",
                    projectName, description, projectId);
            }

            txn.commit();
            Log::Info("DB_UpdateProject: project {} updated by '{}'", projectId, userName);
            return UpdateProjectResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_UpdateProject: {}", e.what());
            return UpdateProjectResult::Error;
        }
    }

    DeleteProjectResult Database::DeleteProject(int projectId, const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_DeleteProject: Database is not connected!");
            return DeleteProjectResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_DeleteProject: user '{}' not found.", userName);
                return DeleteProjectResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_DeleteProject: project id {} not found.", projectId);
                return DeleteProjectResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_DeleteProject: user '{}' is not leader of project {}.", userName, projectId);
                return DeleteProjectResult::Forbidden;
            }

            // Remove memberships first in case FKs are not ON DELETE CASCADE.
            txn.exec_params("DELETE FROM projectmember WHERE projectid = $1", projectId);
            txn.exec_params("DELETE FROM project WHERE projectid = $1", projectId);

            txn.commit();
            Log::Info("DB_DeleteProject: project {} deleted by '{}'", projectId, userName);
            return DeleteProjectResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_DeleteProject: {}", e.what());
            return DeleteProjectResult::Error;
        }
    }
    bool Database::GetProjectMembers(int projectId, std::vector<MemberInfo>& outMembers)
    {
        outMembers.clear();

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_GetProjectMembers: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            pqxx::result res = txn.exec_params(
                "SELECT pm.membershipid, u.username, SUBSTR(pm.joindate::text, 1, 10), pm.role "
                "FROM users u "
                "JOIN projectmember pm ON u.userid = pm.userid "
                "WHERE pm.projectid = $1 "
                "ORDER BY pm.joindate ASC",
                projectId
            );

            for (const auto& row : res)
            {
                MemberInfo info;
                info.MembershipId = row[0].as<int>();
                info.Name = row[1].as<std::string>();
                info.JoinDate = row[2].as<std::string>();
                info.Role = row[3].as<std::string>();
                outMembers.push_back(info);
            }

            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_GetProjectMembers: {}", e.what());
            return false;
        }
    }

    RemoveMemberResult Database::RemoveMember(int projectId, const std::string& memberName,
        const std::string& actorUserName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_RemoveMember: Database is not connected!");
            return RemoveMemberResult::Error;
        }

        if (memberName.empty())
        {
            Log::Warn("DB_RemoveMember: empty member name.");
            return RemoveMemberResult::MemberNotFound;
        }

        // Leader cannot remove themselves from the project.
        if (memberName == actorUserName)
        {
            Log::Warn("DB_RemoveMember: user '{}' cannot remove themselves from project {}.",
                actorUserName, projectId);
            return RemoveMemberResult::CannotRemoveSelf;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // Verify actor exists and is a leader of this project.
            std::string actorUserId;
            std::string actorRole;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, actorUserName, actorUserId, actorRole, projectExists))
            {
                Log::Error("DB_RemoveMember: actor '{}' not found.", actorUserName);
                return RemoveMemberResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_RemoveMember: project id {} not found.", projectId);
                return RemoveMemberResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(actorRole))
            {
                Log::Warn("DB_RemoveMember: user '{}' is not leader of project {}.", actorUserName, projectId);
                return RemoveMemberResult::Forbidden;
            }

            // Resolve the target member and confirm they belong to the project.
            pqxx::result targetUserRes = txn.exec_params(
                "SELECT userid FROM users WHERE username = $1", memberName);
            if (targetUserRes.empty())
            {
                Log::Warn("DB_RemoveMember: target user '{}' not found.", memberName);
                return RemoveMemberResult::MemberNotFound;
            }
            const std::string targetUserId = targetUserRes[0][0].as<std::string>();

            pqxx::result membershipRes = txn.exec_params(
                "SELECT role FROM projectmember WHERE projectid = $1 AND userid = $2",
                projectId, targetUserId);
            if (membershipRes.empty())
            {
                Log::Warn("DB_RemoveMember: '{}' is not a member of project {}.", memberName, projectId);
                return RemoveMemberResult::MemberNotFound;
            }

            // Do not allow removing another leader (keeps project leadership intact).
            const std::string targetRole = membershipRes[0][0].as<std::string>();
            if (IsLeaderRoleString(targetRole))
            {
                Log::Warn("DB_RemoveMember: cannot remove leader '{}' from project {}.", memberName, projectId);
                return RemoveMemberResult::Forbidden;
            }

            txn.exec_params(
                "DELETE FROM projectmember WHERE projectid = $1 AND userid = $2",
                projectId, targetUserId);

            txn.commit();
            Log::Info("DB_RemoveMember: '{}' removed '{}' from project {}.",
                actorUserName, memberName, projectId);
            return RemoveMemberResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_RemoveMember: {}", e.what());
            return RemoveMemberResult::Error;
        }
    }

    InsertMilestoneResult Database::InsertMilestone(int projectId, const std::string& milestoneName, const std::string& startDate, const std::string& endDate,
        float progressPercentage, const std::string& status, const std::string& userName, int& outMilestoneId)
    {
        outMilestoneId = 0;

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_InsertMilestone: Database is not connected!");
            return InsertMilestoneResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_InsertMilestone: user '{}' not found.", userName);
                return InsertMilestoneResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_InsertMilestone: project id {} not found.", projectId);
                return InsertMilestoneResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_InsertMilestone: user '{}' is not leader of project {}.", userName, projectId);
                return InsertMilestoneResult::Forbidden;
            }

            // Dates are MM-DD-YYYY; PostgreSQL DateStyle MDY accepts them into date columns.
            // to_date is used for a stable cast regardless of session DateStyle.
            pqxx::result res = txn.exec_params(
                "INSERT INTO milestone (projectid, milestonename, startdate, enddate, progresspercentage, status) "
                "VALUES ($1, $2, to_date($3, 'MM-DD-YYYY'), to_date($4, 'MM-DD-YYYY'), $5, $6) "
                "RETURNING milestoneid",
                projectId, milestoneName, startDate, endDate, progressPercentage, status);

            if (res.empty())
            {
                Log::Error("DB_InsertMilestone: insert returned no id.");
                return InsertMilestoneResult::Error;
            }

            outMilestoneId = res[0][0].as<int>();
            txn.commit();

            Log::Info("DB_InsertMilestone: milestone '{}' (id {}) created on project {} by '{}'",
                milestoneName, outMilestoneId, projectId, userName);
            return InsertMilestoneResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_InsertMilestone: {}", e.what());
            return InsertMilestoneResult::Error;
        }
    }

    bool Database::GetMilestonesForProject(int projectId, std::vector<MilestoneInfo>& outMilestones)
    {
        outMilestones.clear();

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_GetMilestonesForProject: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            pqxx::result res = txn.exec_params(
                "SELECT milestoneid, projectid, milestonename, "
                "to_char(startdate, 'MM-DD-YYYY') AS startdate, "
                "to_char(enddate, 'MM-DD-YYYY') AS enddate, "
                "progresspercentage, status "
                "FROM milestone "
                "WHERE projectid = $1 "
                "ORDER BY startdate ASC NULLS LAST, milestoneid ASC",
                projectId);

            outMilestones.reserve(static_cast<size_t>(res.size()));
            for (const auto& row : res)
            {
                MilestoneInfo info;
                info.Id = row["milestoneid"].as<int>();
                info.ProjectId = row["projectid"].as<int>();
                info.Name = row["milestonename"].as<std::string>();
                if (!row["startdate"].is_null())
                    info.StartDate = row["startdate"].as<std::string>();
                if (!row["enddate"].is_null())
                    info.EndDate = row["enddate"].as<std::string>();
                if (!row["progresspercentage"].is_null())
                    info.ProgressPercentage = row["progresspercentage"].as<float>();
                if (!row["status"].is_null())
                    info.Status = row["status"].as<std::string>();
                outMilestones.push_back(std::move(info));
            }

            Log::Info("DB_GetMilestonesForProject: loaded {} milestone(s) for project {}",
                outMilestones.size(), projectId);
            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_GetMilestonesForProject: {}", e.what());
            outMilestones.clear();
            return false;
        }
    }

    namespace
    {
        bool EqualsIgnoreCaseLocal(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size())
                return false;
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i]))
                    != std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }

        // Deduplicate, reject self, ensure each dep is a task on projectId, reject cycles.
        // On success writes rows into taskdependency for taskId (caller should delete old rows first if updating).
        bool ApplyTaskDependencies(pqxx::work& txn, int projectId, int taskId,
            const std::vector<int>& dependsOnTaskIds, std::string& outError)
        {
            outError.clear();

            std::unordered_set<int> uniqueDeps;
            uniqueDeps.reserve(dependsOnTaskIds.size());
            for (int depId : dependsOnTaskIds)
            {
                if (depId <= 0)
                    continue;
                if (depId == taskId)
                {
                    outError = "A task cannot depend on itself.";
                    return false;
                }
                uniqueDeps.insert(depId);
            }

            for (int depId : uniqueDeps)
            {
                pqxx::result depRes = txn.exec_params(
                    "SELECT 1 FROM task t "
                    "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                    "WHERE t.taskid = $1 AND m.projectid = $2",
                    depId, projectId);
                if (depRes.empty())
                {
                    outError = "Dependency task is not part of this project.";
                    return false;
                }

                // Cycle: if depId (transitively) already depends on taskId, adding taskId→depId loops.
                pqxx::result cycleRes = txn.exec_params(
                    "WITH RECURSIVE chain AS ("
                    "  SELECT dependsontaskid FROM taskdependency WHERE taskid = $1 "
                    "  UNION "
                    "  SELECT td.dependsontaskid FROM taskdependency td "
                    "  INNER JOIN chain c ON td.taskid = c.dependsontaskid"
                    ") "
                    "SELECT 1 FROM chain WHERE dependsontaskid = $2 LIMIT 1",
                    depId, taskId);
                if (!cycleRes.empty())
                {
                    outError = "Dependency would create a circular dependency.";
                    return false;
                }
            }

            for (int depId : uniqueDeps)
            {
                txn.exec_params(
                    "INSERT INTO taskdependency (taskid, dependsontaskid) VALUES ($1, $2)",
                    taskId, depId);
            }

            return true;
        }

        // True when every prerequisite of taskId has status "done" (or there are none).
        bool AreTaskDependenciesSatisfied(pqxx::work& txn, int taskId)
        {
            pqxx::result res = txn.exec_params(
                "SELECT COUNT(*)::int AS blocked "
                "FROM taskdependency td "
                "INNER JOIN task dep ON dep.taskid = td.dependsontaskid "
                "WHERE td.taskid = $1 "
                "AND (dep.taskstatus IS NULL OR LOWER(dep.taskstatus) <> 'done')",
                taskId);
            if (res.empty())
                return true;
            return res[0]["blocked"].as<int>() == 0;
        }

        // Names of incomplete prerequisites (for log / UI messages).
        std::string FormatUnmetDependencyNames(pqxx::work& txn, int taskId)
        {
            pqxx::result res = txn.exec_params(
                "SELECT dep.taskname "
                "FROM taskdependency td "
                "INNER JOIN task dep ON dep.taskid = td.dependsontaskid "
                "WHERE td.taskid = $1 "
                "AND (dep.taskstatus IS NULL OR LOWER(dep.taskstatus) <> 'done') "
                "ORDER BY dep.taskid ASC",
                taskId);

            std::string names;
            for (const auto& row : res)
            {
                if (!names.empty())
                    names += ", ";
                if (!row["taskname"].is_null())
                    names += row["taskname"].as<std::string>();
                else
                    names += "(unnamed)";
            }
            return names;
        }
    }

    InsertTaskResult Database::InsertTask(int projectId, int milestoneId, int assignedMembershipId,
        const std::string& taskName, const std::string& description, float estimatedHours,
        const std::string& priority, const std::string& deadline, const std::string& status,
        const std::vector<int>& dependsOnTaskIds, const std::string& userName, int& outTaskId)
    {
        outTaskId = 0;

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_InsertTask: Database is not connected!");
            return InsertTaskResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_InsertTask: user '{}' not found.", userName);
                return InsertTaskResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_InsertTask: project id {} not found.", projectId);
                return InsertTaskResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_InsertTask: user '{}' is not leader of project {}.", userName, projectId);
                return InsertTaskResult::Forbidden;
            }

            // Milestone must belong to this project.
            pqxx::result milestoneRes = txn.exec_params(
                "SELECT 1 FROM milestone WHERE milestoneid = $1 AND projectid = $2",
                milestoneId, projectId);
            if (milestoneRes.empty())
            {
                Log::Warn("DB_InsertTask: milestone {} not found on project {}.", milestoneId, projectId);
                return InsertTaskResult::MilestoneNotFound;
            }

            // Optional assignee must be a membership on this project.
            if (assignedMembershipId > 0)
            {
                pqxx::result memberRes = txn.exec_params(
                    "SELECT 1 FROM projectmember WHERE membershipid = $1 AND projectid = $2",
                    assignedMembershipId, projectId);
                if (memberRes.empty())
                {
                    Log::Warn("DB_InsertTask: membership {} not on project {}.", assignedMembershipId, projectId);
                    return InsertTaskResult::MemberNotFound;
                }
            }

            // Branch on optional assignee / deadline so NULL columns stay NULL in SQL.
            pqxx::result res;
            if (assignedMembershipId > 0 && !deadline.empty())
            {
                res = txn.exec_params(
                    "INSERT INTO task (milestoneid, assignedmemberid, estimatedhours, priority, deadline, taskstatus, taskname, taskdescription) "
                    "VALUES ($1, $2, $3, $4, to_date($5, 'MM-DD-YYYY'), $6, $7, NULLIF($8, '')) "
                    "RETURNING taskid",
                    milestoneId, assignedMembershipId, estimatedHours, priority, deadline, status, taskName, description);
            }
            else if (assignedMembershipId > 0 && deadline.empty())
            {
                res = txn.exec_params(
                    "INSERT INTO task (milestoneid, assignedmemberid, estimatedhours, priority, deadline, taskstatus, taskname, taskdescription) "
                    "VALUES ($1, $2, $3, $4, NULL, $5, $6, NULLIF($7, '')) "
                    "RETURNING taskid",
                    milestoneId, assignedMembershipId, estimatedHours, priority, status, taskName, description);
            }
            else if (assignedMembershipId <= 0 && !deadline.empty())
            {
                res = txn.exec_params(
                    "INSERT INTO task (milestoneid, assignedmemberid, estimatedhours, priority, deadline, taskstatus, taskname, taskdescription) "
                    "VALUES ($1, NULL, $2, $3, to_date($4, 'MM-DD-YYYY'), $5, $6, NULLIF($7, '')) "
                    "RETURNING taskid",
                    milestoneId, estimatedHours, priority, deadline, status, taskName, description);
            }
            else
            {
                res = txn.exec_params(
                    "INSERT INTO task (milestoneid, assignedmemberid, estimatedhours, priority, deadline, taskstatus, taskname, taskdescription) "
                    "VALUES ($1, NULL, $2, $3, NULL, $4, $5, NULLIF($6, '')) "
                    "RETURNING taskid",
                    milestoneId, estimatedHours, priority, status, taskName, description);
            }

            if (res.empty())
            {
                Log::Error("DB_InsertTask: insert returned no id.");
                return InsertTaskResult::Error;
            }

            outTaskId = res[0][0].as<int>();

            std::string depError;
            if (!ApplyTaskDependencies(txn, projectId, outTaskId, dependsOnTaskIds, depError))
            {
                Log::Warn("DB_InsertTask: invalid dependencies for new task: {}", depError);
                outTaskId = 0;
                return InsertTaskResult::InvalidDependency;
            }

            txn.commit();

            Log::Info("DB_InsertTask: task '{}' (id {}) created on project {} by '{}' ({} deps)",
                taskName, outTaskId, projectId, userName, dependsOnTaskIds.size());
            return InsertTaskResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_InsertTask: {}", e.what());
            outTaskId = 0;
            return InsertTaskResult::Error;
        }
    }

    bool Database::GetTasksForProject(int projectId, std::vector<TaskInfo>& outTasks)
    {
        outTasks.clear();

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_GetTasksForProject: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // Tasks hang off milestones; join assignee username when present.
            pqxx::result res = txn.exec_params(
                "SELECT t.taskid, t.milestoneid, m.milestonename, "
                "t.assignedmemberid, u.username AS assignedname, "
                "t.taskname, t.taskdescription, t.estimatedhours, t.priority, "
                "to_char(t.deadline, 'MM-DD-YYYY') AS deadline, "
                "t.taskstatus, t.reviewcomment "
                "FROM task t "
                "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                "LEFT JOIN projectmember pm ON t.assignedmemberid = pm.membershipid "
                "LEFT JOIN users u ON pm.userid = u.userid "
                "WHERE m.projectid = $1 "
                "ORDER BY m.startdate ASC NULLS LAST, m.milestoneid ASC, t.taskid ASC",
                projectId);

            outTasks.reserve(static_cast<size_t>(res.size()));
            std::unordered_map<int, size_t> taskIndexById;
            taskIndexById.reserve(static_cast<size_t>(res.size()));

            for (const auto& row : res)
            {
                TaskInfo info;
                info.Id = row["taskid"].as<int>();
                info.MilestoneId = row["milestoneid"].as<int>();
                if (!row["milestonename"].is_null())
                    info.MilestoneName = row["milestonename"].as<std::string>();
                if (!row["assignedmemberid"].is_null())
                    info.AssignedMembershipId = row["assignedmemberid"].as<int>();
                if (!row["assignedname"].is_null())
                    info.AssignedMemberName = row["assignedname"].as<std::string>();
                info.Name = row["taskname"].as<std::string>();
                if (!row["taskdescription"].is_null())
                    info.Description = row["taskdescription"].as<std::string>();
                if (!row["estimatedhours"].is_null())
                    info.EstimatedHours = row["estimatedhours"].as<float>();
                if (!row["priority"].is_null())
                    info.Priority = row["priority"].as<std::string>();
                if (!row["deadline"].is_null())
                    info.Deadline = row["deadline"].as<std::string>();
                if (!row["taskstatus"].is_null())
                    info.Status = row["taskstatus"].as<std::string>();
                if (!row["reviewcomment"].is_null())
                    info.ReviewComment = row["reviewcomment"].as<std::string>();
                info.DependenciesSatisfied = true;
                taskIndexById[info.Id] = outTasks.size();
                outTasks.push_back(std::move(info));
            }

            // Load taskdependency for all tasks on this project in one query.
            if (!outTasks.empty())
            {
                pqxx::result depRes = txn.exec_params(
                    "SELECT td.taskid, td.dependsontaskid, dep.taskname, dep.taskstatus "
                    "FROM taskdependency td "
                    "INNER JOIN task t ON t.taskid = td.taskid "
                    "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                    "INNER JOIN task dep ON dep.taskid = td.dependsontaskid "
                    "WHERE m.projectid = $1 "
                    "ORDER BY td.taskid ASC, td.dependsontaskid ASC",
                    projectId);

                for (const auto& row : depRes)
                {
                    const int taskId = row["taskid"].as<int>();
                    auto it = taskIndexById.find(taskId);
                    if (it == taskIndexById.end())
                        continue;

                    TaskDependencyInfo dep;
                    dep.DependsOnTaskId = row["dependsontaskid"].as<int>();
                    if (!row["taskname"].is_null())
                        dep.DependsOnTaskName = row["taskname"].as<std::string>();
                    if (!row["taskstatus"].is_null())
                        dep.DependsOnTaskStatus = row["taskstatus"].as<std::string>();
                    dep.IsDone = EqualsIgnoreCaseLocal(dep.DependsOnTaskStatus, "done");

                    TaskInfo& task = outTasks[it->second];
                    if (!dep.IsDone)
                        task.DependenciesSatisfied = false;
                    task.Dependencies.push_back(std::move(dep));
                }
            }

            Log::Info("DB_GetTasksForProject: loaded {} task(s) for project {}", outTasks.size(), projectId);
            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_GetTasksForProject: {}", e.what());
            outTasks.clear();
            return false;
        }
    }

    UpdateTaskResult Database::UpdateTask(int projectId, int taskId, int milestoneId, int assignedMembershipId,
        const std::string& taskName, const std::string& description, float estimatedHours,
        const std::string& priority, const std::string& deadline, const std::string& status,
        const std::vector<int>& dependsOnTaskIds, const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_UpdateTask: Database is not connected!");
            return UpdateTaskResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_UpdateTask: user '{}' not found.", userName);
                return UpdateTaskResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_UpdateTask: project id {} not found.", projectId);
                return UpdateTaskResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_UpdateTask: user '{}' is not leader of project {}.", userName, projectId);
                return UpdateTaskResult::Forbidden;
            }

            // Task must belong to this project (via its current milestone).
            pqxx::result taskRes = txn.exec_params(
                "SELECT 1 FROM task t "
                "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                "WHERE t.taskid = $1 AND m.projectid = $2",
                taskId, projectId);
            if (taskRes.empty())
            {
                Log::Warn("DB_UpdateTask: task {} not found on project {}.", taskId, projectId);
                return UpdateTaskResult::TaskNotFound;
            }

            pqxx::result milestoneRes = txn.exec_params(
                "SELECT 1 FROM milestone WHERE milestoneid = $1 AND projectid = $2",
                milestoneId, projectId);
            if (milestoneRes.empty())
            {
                Log::Warn("DB_UpdateTask: milestone {} not found on project {}.", milestoneId, projectId);
                return UpdateTaskResult::MilestoneNotFound;
            }

            if (assignedMembershipId > 0)
            {
                pqxx::result memberRes = txn.exec_params(
                    "SELECT 1 FROM projectmember WHERE membershipid = $1 AND projectid = $2",
                    assignedMembershipId, projectId);
                if (memberRes.empty())
                {
                    Log::Warn("DB_UpdateTask: membership {} not on project {}.", assignedMembershipId, projectId);
                    return UpdateTaskResult::MemberNotFound;
                }
            }

            if (assignedMembershipId > 0 && !deadline.empty())
            {
                txn.exec_params(
                    "UPDATE task SET milestoneid = $1, assignedmemberid = $2, estimatedhours = $3, "
                    "priority = $4, deadline = to_date($5, 'MM-DD-YYYY'), taskstatus = $6, "
                    "taskname = $7, taskdescription = NULLIF($8, '') "
                    "WHERE taskid = $9",
                    milestoneId, assignedMembershipId, estimatedHours, priority, deadline, status,
                    taskName, description, taskId);
            }
            else if (assignedMembershipId > 0 && deadline.empty())
            {
                txn.exec_params(
                    "UPDATE task SET milestoneid = $1, assignedmemberid = $2, estimatedhours = $3, "
                    "priority = $4, deadline = NULL, taskstatus = $5, "
                    "taskname = $6, taskdescription = NULLIF($7, '') "
                    "WHERE taskid = $8",
                    milestoneId, assignedMembershipId, estimatedHours, priority, status,
                    taskName, description, taskId);
            }
            else if (assignedMembershipId <= 0 && !deadline.empty())
            {
                txn.exec_params(
                    "UPDATE task SET milestoneid = $1, assignedmemberid = NULL, estimatedhours = $2, "
                    "priority = $3, deadline = to_date($4, 'MM-DD-YYYY'), taskstatus = $5, "
                    "taskname = $6, taskdescription = NULLIF($7, '') "
                    "WHERE taskid = $8",
                    milestoneId, estimatedHours, priority, deadline, status,
                    taskName, description, taskId);
            }
            else
            {
                txn.exec_params(
                    "UPDATE task SET milestoneid = $1, assignedmemberid = NULL, estimatedhours = $2, "
                    "priority = $3, deadline = NULL, taskstatus = $4, "
                    "taskname = $5, taskdescription = NULLIF($6, '') "
                    "WHERE taskid = $7",
                    milestoneId, estimatedHours, priority, status,
                    taskName, description, taskId);
            }

            // Replace full dependency set for this task.
            txn.exec_params("DELETE FROM taskdependency WHERE taskid = $1", taskId);

            std::string depError;
            if (!ApplyTaskDependencies(txn, projectId, taskId, dependsOnTaskIds, depError))
            {
                Log::Warn("DB_UpdateTask: invalid dependencies for task {}: {}", taskId, depError);
                return UpdateTaskResult::InvalidDependency;
            }

            txn.commit();
            Log::Info("DB_UpdateTask: task {} updated on project {} by '{}' ({} deps)",
                taskId, projectId, userName, dependsOnTaskIds.size());
            return UpdateTaskResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_UpdateTask: {}", e.what());
            return UpdateTaskResult::Error;
        }
    }

    DeleteTaskResult Database::DeleteTask(int projectId, int taskId, const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_DeleteTask: Database is not connected!");
            return DeleteTaskResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_DeleteTask: user '{}' not found.", userName);
                return DeleteTaskResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_DeleteTask: project id {} not found.", projectId);
                return DeleteTaskResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_DeleteTask: user '{}' is not leader of project {}.", userName, projectId);
                return DeleteTaskResult::Forbidden;
            }

            pqxx::result taskRes = txn.exec_params(
                "SELECT 1 FROM task t "
                "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                "WHERE t.taskid = $1 AND m.projectid = $2",
                taskId, projectId);
            if (taskRes.empty())
            {
                Log::Warn("DB_DeleteTask: task {} not found on project {}.", taskId, projectId);
                return DeleteTaskResult::TaskNotFound;
            }

            // Dependent rows cascade via FKs (deliverablelog, taskdependency).
            txn.exec_params("DELETE FROM task WHERE taskid = $1", taskId);
            txn.commit();

            Log::Info("DB_DeleteTask: task {} deleted from project {} by '{}'", taskId, projectId, userName);
            return DeleteTaskResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_DeleteTask: {}", e.what());
            return DeleteTaskResult::Error;
        }
    }

    namespace
    {
        // Resolves task + verifies it belongs to projectId. Fills assigned membership id and status.
        // Returns false if task is not on the project.
        bool LoadTaskOnProject(pqxx::work& txn, int projectId, int taskId,
            int& outAssignedMembershipId, std::string& outStatus)
        {
            outAssignedMembershipId = 0;
            outStatus.clear();

            pqxx::result res = txn.exec_params(
                "SELECT t.assignedmemberid, t.taskstatus "
                "FROM task t "
                "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                "WHERE t.taskid = $1 AND m.projectid = $2",
                taskId, projectId);
            if (res.empty())
                return false;

            if (!res[0]["assignedmemberid"].is_null())
                outAssignedMembershipId = res[0]["assignedmemberid"].as<int>();
            if (!res[0]["taskstatus"].is_null())
                outStatus = res[0]["taskstatus"].as<std::string>();
            return true;
        }

        // Membership id for userName on projectId, or 0 if not a member / not found.
        int ResolveMembershipId(pqxx::work& txn, int projectId, const std::string& userName, bool& outUserFound)
        {
            outUserFound = false;
            pqxx::result userRes = txn.exec_params("SELECT userid FROM users WHERE username = $1", userName);
            if (userRes.empty())
                return 0;
            outUserFound = true;
            const std::string userId = userRes[0][0].as<std::string>();

            pqxx::result memRes = txn.exec_params(
                "SELECT membershipid FROM projectmember WHERE projectid = $1 AND userid = $2",
                projectId, userId);
            if (memRes.empty())
                return 0;
            return memRes[0][0].as<int>();
        }

        bool EqualsIgnoreCase(const std::string& a, const std::string& b)
        {
            if (a.size() != b.size())
                return false;
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i]))
                    != std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }
    }

    AcceptTaskResult Database::AcceptTask(int projectId, int taskId, const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_AcceptTask: Database is not connected!");
            return AcceptTaskResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // Ensure project exists (also used for membership check path).
            pqxx::result projectRes = txn.exec_params("SELECT 1 FROM project WHERE projectid = $1", projectId);
            if (projectRes.empty())
            {
                Log::Warn("DB_AcceptTask: project id {} not found.", projectId);
                return AcceptTaskResult::ProjectNotFound;
            }

            bool userFound = false;
            const int membershipId = ResolveMembershipId(txn, projectId, userName, userFound);
            if (!userFound)
            {
                Log::Error("DB_AcceptTask: user '{}' not found.", userName);
                return AcceptTaskResult::UserNotFound;
            }

            int assignedMembershipId = 0;
            std::string status;
            if (!LoadTaskOnProject(txn, projectId, taskId, assignedMembershipId, status))
            {
                Log::Warn("DB_AcceptTask: task {} not found on project {}.", taskId, projectId);
                return AcceptTaskResult::TaskNotFound;
            }

            if (membershipId <= 0 || assignedMembershipId <= 0 || membershipId != assignedMembershipId)
            {
                Log::Warn("DB_AcceptTask: user '{}' is not the assignee of task {}.", userName, taskId);
                return AcceptTaskResult::Forbidden;
            }

            if (!EqualsIgnoreCase(status, "pending"))
            {
                Log::Warn("DB_AcceptTask: task {} status '{}' is not pending.", taskId, status);
                return AcceptTaskResult::InvalidStatus;
            }

            // Prerequisites (taskdependency) must all be done before work can start.
            if (!AreTaskDependenciesSatisfied(txn, taskId))
            {
                const std::string blockedBy = FormatUnmetDependencyNames(txn, taskId);
                Log::Warn("DB_AcceptTask: task {} blocked by incomplete dependencies: {}", taskId, blockedBy);
                return AcceptTaskResult::DependenciesBlocked;
            }

            txn.exec_params(
                "UPDATE task SET taskstatus = $1 WHERE taskid = $2",
                "in progress", taskId);
            txn.commit();

            Log::Info("DB_AcceptTask: task {} accepted by '{}'", taskId, userName);
            return AcceptTaskResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_AcceptTask: {}", e.what());
            return AcceptTaskResult::Error;
        }
    }

    SubmitTaskResult Database::SubmitTask(int projectId, int taskId,
        const std::string& executionNotes, const std::string& filePath,
        const std::string& codeSnippet, const std::string& submissionTime,
        const std::string& userName, int& outLogId)
    {
        outLogId = 0;

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_SubmitTask: Database is not connected!");
            return SubmitTaskResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            pqxx::result projectRes = txn.exec_params("SELECT 1 FROM project WHERE projectid = $1", projectId);
            if (projectRes.empty())
            {
                Log::Warn("DB_SubmitTask: project id {} not found.", projectId);
                return SubmitTaskResult::ProjectNotFound;
            }

            bool userFound = false;
            const int membershipId = ResolveMembershipId(txn, projectId, userName, userFound);
            if (!userFound)
            {
                Log::Error("DB_SubmitTask: user '{}' not found.", userName);
                return SubmitTaskResult::UserNotFound;
            }

            int assignedMembershipId = 0;
            std::string status;
            if (!LoadTaskOnProject(txn, projectId, taskId, assignedMembershipId, status))
            {
                Log::Warn("DB_SubmitTask: task {} not found on project {}.", taskId, projectId);
                return SubmitTaskResult::TaskNotFound;
            }

            if (membershipId <= 0 || assignedMembershipId <= 0 || membershipId != assignedMembershipId)
            {
                Log::Warn("DB_SubmitTask: user '{}' is not the assignee of task {}.", userName, taskId);
                return SubmitTaskResult::Forbidden;
            }

            if (!EqualsIgnoreCase(status, "in progress"))
            {
                Log::Warn("DB_SubmitTask: task {} status '{}' is not in progress.", taskId, status);
                return SubmitTaskResult::InvalidStatus;
            }

            // Defense in depth: still require prerequisites even if status was forced earlier.
            if (!AreTaskDependenciesSatisfied(txn, taskId))
            {
                const std::string blockedBy = FormatUnmetDependencyNames(txn, taskId);
                Log::Warn("DB_SubmitTask: task {} blocked by incomplete dependencies: {}", taskId, blockedBy);
                return SubmitTaskResult::DependenciesBlocked;
            }

            // Persist deliverable + move task to under review in one transaction.
            pqxx::result logRes = txn.exec_params(
                "INSERT INTO deliverablelog (taskid, executionnotes, filepath, projectcodesnippet, submissiontime) "
                "VALUES ($1, $2, $3, $4, to_timestamp($5, 'YYYY-MM-DD HH24:MI:SS')) "
                "RETURNING logid",
                taskId, executionNotes, filePath, codeSnippet, submissionTime);

            if (logRes.empty())
            {
                Log::Error("DB_SubmitTask: deliverable insert returned no id.");
                return SubmitTaskResult::Error;
            }

            outLogId = logRes[0][0].as<int>();

            txn.exec_params(
                "UPDATE task SET taskstatus = $1 WHERE taskid = $2",
                "under review", taskId);

            txn.commit();

            Log::Info("DB_SubmitTask: task {} submitted by '{}' (log {}, time {})",
                taskId, userName, outLogId, submissionTime);
            return SubmitTaskResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_SubmitTask: {}", e.what());
            return SubmitTaskResult::Error;
        }
    }

    bool Database::GetLatestDeliverableForTask(int projectId, int taskId,
        DeliverableInfo& outDeliverable, bool& outFound)
    {
        outDeliverable = DeliverableInfo{};
        outFound = false;

        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_GetLatestDeliverableForTask: Database is not connected!");
            return false;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            // Ensure task belongs to the project, then take the newest deliverable log.
            pqxx::result res = txn.exec_params(
                "SELECT d.logid, d.taskid, d.executionnotes, d.filepath, d.projectcodesnippet, "
                "to_char(d.submissiontime, 'YYYY-MM-DD HH24:MI:SS') AS submissiontime "
                "FROM deliverablelog d "
                "INNER JOIN task t ON d.taskid = t.taskid "
                "INNER JOIN milestone m ON t.milestoneid = m.milestoneid "
                "WHERE d.taskid = $1 AND m.projectid = $2 "
                "ORDER BY d.logid DESC "
                "LIMIT 1",
                taskId, projectId);

            if (res.empty())
            {
                outFound = false;
                return true;
            }

            const auto& row = res[0];
            outDeliverable.LogId = row["logid"].as<int>();
            outDeliverable.TaskId = row["taskid"].as<int>();
            if (!row["executionnotes"].is_null())
                outDeliverable.ExecutionNotes = row["executionnotes"].as<std::string>();
            if (!row["filepath"].is_null())
                outDeliverable.FilePath = row["filepath"].as<std::string>();
            if (!row["projectcodesnippet"].is_null())
                outDeliverable.CodeSnippet = row["projectcodesnippet"].as<std::string>();
            if (!row["submissiontime"].is_null())
                outDeliverable.SubmissionTime = row["submissiontime"].as<std::string>();

            outFound = true;
            return true;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_GetLatestDeliverableForTask: {}", e.what());
            outDeliverable = DeliverableInfo{};
            outFound = false;
            return false;
        }
    }

    ReviewTaskResult Database::ReviewTask(int projectId, int taskId, bool approved,
        const std::string& reviewComment, const std::string& userName)
    {
        if (!s_Connection || !s_Connection->is_open())
        {
            Log::Error("DB_ReviewTask: Database is not connected!");
            return ReviewTaskResult::Error;
        }

        try
        {
            pqxx::work txn(*s_Connection);

            std::string userId;
            std::string role;
            bool projectExists = false;
            if (!ResolveUserAndRole(txn, projectId, userName, userId, role, projectExists))
            {
                Log::Error("DB_ReviewTask: user '{}' not found.", userName);
                return ReviewTaskResult::UserNotFound;
            }
            if (!projectExists)
            {
                Log::Warn("DB_ReviewTask: project id {} not found.", projectId);
                return ReviewTaskResult::ProjectNotFound;
            }
            if (!IsLeaderRoleString(role))
            {
                Log::Warn("DB_ReviewTask: user '{}' is not leader of project {}.", userName, projectId);
                return ReviewTaskResult::Forbidden;
            }

            int assignedMembershipId = 0;
            std::string status;
            if (!LoadTaskOnProject(txn, projectId, taskId, assignedMembershipId, status))
            {
                Log::Warn("DB_ReviewTask: task {} not found on project {}.", taskId, projectId);
                return ReviewTaskResult::TaskNotFound;
            }

            if (!EqualsIgnoreCase(status, "under review"))
            {
                Log::Warn("DB_ReviewTask: task {} status '{}' is not under review.", taskId, status);
                return ReviewTaskResult::InvalidStatus;
            }

            // Approving marks the task done — only allow when prerequisites are complete.
            if (approved && !AreTaskDependenciesSatisfied(txn, taskId))
            {
                const std::string blockedBy = FormatUnmetDependencyNames(txn, taskId);
                Log::Warn("DB_ReviewTask: cannot approve task {} — blocked by: {}", taskId, blockedBy);
                return ReviewTaskResult::DependenciesBlocked;
            }

            const std::string newStatus = approved ? "done" : "in progress";

            if (reviewComment.empty())
            {
                txn.exec_params(
                    "UPDATE task SET taskstatus = $1, reviewcomment = NULL WHERE taskid = $2",
                    newStatus, taskId);
            }
            else
            {
                txn.exec_params(
                    "UPDATE task SET taskstatus = $1, reviewcomment = $2 WHERE taskid = $3",
                    newStatus, reviewComment, taskId);
            }

            txn.commit();
            Log::Info("DB_ReviewTask: task {} {} by '{}' → status '{}'",
                taskId, approved ? "approved" : "rejected", userName, newStatus);
            return ReviewTaskResult::Success;
        }
        catch (const std::exception& e)
        {
            Log::Error("DB_ReviewTask: {}", e.what());
            return ReviewTaskResult::Error;
        }
    }

} // namespace TrackingTool
