#include "TimeUtils.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace TrackingTool
{
    namespace Utils
    {
        std::string GetDateTime()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);

#if defined(_WIN32)
            struct tm timeinfo;
            localtime_s(&timeinfo, &now_c);
            std::tm* now_tm = &timeinfo;
#else
            std::tm* now_tm = std::localtime(&now_c);
#endif

            std::ostringstream ss;
            // Format as MM-DD-YYYY
            ss << std::put_time(now_tm, "%m-%d-%Y");
            return ss.str();
        }
    }
}
