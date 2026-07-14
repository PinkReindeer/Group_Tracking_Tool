#include "TimeUtils.h"

#include <chrono>
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace TrackingTool
{
    namespace Utils
    {
        namespace
        {
            bool ParseMmDdYyyy(const std::string& s, int& outMonth, int& outDay, int& outYear)
            {
                outMonth = outDay = outYear = 0;
                if (s.empty())
                    return false;

                int month = 0, day = 0, year = 0;
                // Accept MM-DD-YYYY with optional single-digit month/day.
#if defined(_WIN32)
                if (sscanf_s(s.c_str(), "%d-%d-%d", &month, &day, &year) != 3)
                    return false;
#else
                if (std::sscanf(s.c_str(), "%d-%d-%d", &month, &day, &year) != 3)
                    return false;
#endif

                if (year < 1900 || year > 2100)
                    return false;
                if (month < 1 || month > 12)
                    return false;
                if (day < 1 || day > 31)
                    return false;

                static constexpr int kDaysInMonth[] = {
                    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
                };
                int maxDay = kDaysInMonth[month];
                if (month == 2)
                {
                    const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                    if (leap)
                        maxDay = 29;
                }
                if (day > maxDay)
                    return false;

                outMonth = month;
                outDay = day;
                outYear = year;
                return true;
            }

            // Normalize to YYYYMMDD integer for safe ordering.
            int ToSortableYmd(int month, int day, int year)
            {
                return year * 10000 + month * 100 + day;
            }
        }

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

        bool IsValidMmDdYyyy(const std::string& s)
        {
            int m = 0, d = 0, y = 0;
            return ParseMmDdYyyy(s, m, d, y);
        }

        int CompareMmDdYyyy(const std::string& a, const std::string& b)
        {
            int am = 0, ad = 0, ay = 0;
            int bm = 0, bd = 0, by = 0;
            if (!ParseMmDdYyyy(a, am, ad, ay) || !ParseMmDdYyyy(b, bm, bd, by))
                return 0;

            const int av = ToSortableYmd(am, ad, ay);
            const int bv = ToSortableYmd(bm, bd, by);
            if (av < bv) return -1;
            if (av > bv) return 1;
            return 0;
        }

        std::string ComputeMilestoneStatus(float progressPercentage,
            const std::string& startDateMmDdYyyy, const std::string& endDateMmDdYyyy)
        {
            if (progressPercentage >= 100.0f)
                return "completed";

            const std::string today = GetDateTime();

            // Before the planned window → not started.
            if (CompareMmDdYyyy(today, startDateMmDdYyyy) < 0)
                return "not started";

            // Within [start, end] inclusive → in progress.
            if (CompareMmDdYyyy(today, endDateMmDdYyyy) <= 0)
                return "in progress";

            // Past end date but not 100% complete → still treated as in progress.
            return "in progress";
        }
    }
}
