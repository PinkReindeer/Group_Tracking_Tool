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

} // namespace TrackingTool
