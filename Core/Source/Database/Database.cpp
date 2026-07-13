#include "Database.h"

#include <filesystem>
#include <fstream>
#include <string>

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

} // namespace TrackingTool
