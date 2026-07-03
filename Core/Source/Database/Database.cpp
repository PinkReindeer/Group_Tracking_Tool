#include "Database.h"

#include <filesystem>
#include <fstream>
#include <string>

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

    std::string GetDatabaseURI()
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
}
