#include "TimeUtils.h"

#include <chrono>
#include <ctime>
#include <cstdio>

namespace TrackingTool
{
    namespace Utils
    {
        namespace
        {
            bool ParseMmDdYyyy(const char* s, int& outMonth, int& outDay, int& outYear)
            {
                outMonth = outDay = outYear = 0;
                if (!s || s[0] == '\0')
                    return false;

                int month = 0, day = 0, year = 0;
                // Accept MM-DD-YYYY with optional single-digit month/day.
#if defined(_WIN32)
                if (sscanf_s(s, "%d-%d-%d", &month, &day, &year) != 3)
                    return false;
#else
                if (std::sscanf(s, "%d-%d-%d", &month, &day, &year) != 3)
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

            // Fills outTm with local wall-clock time. Returns false on failure.
            bool GetLocalTm(std::tm& outTm)
            {
                const auto now = std::chrono::system_clock::now();
                const std::time_t now_c = std::chrono::system_clock::to_time_t(now);

#if defined(_WIN32)
                if (localtime_s(&outTm, &now_c) != 0)
                    return false;
                return true;
#else
                std::tm* now_tm = std::localtime(&now_c);
                if (!now_tm)
                    return false;
                outTm = *now_tm;
                return true;
#endif
            }
        }

        bool GetDateMmDdYyyy(char* outBuf, size_t outBufSize)
        {
            if (!outBuf || outBufSize < 11)
                return false;

            std::tm timeinfo{};
            if (!GetLocalTm(timeinfo))
            {
                outBuf[0] = '\0';
                return false;
            }

            // MM-DD-YYYY = 10 chars + NUL
            const int written = std::snprintf(outBuf, outBufSize, "%02d-%02d-%04d",
                timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900);
            return written == 10;
        }

        const char* GetTodayMmDdYyyy()
        {
            // Static process-lifetime cache: refresh only when the calendar day changes.
            static char s_Date[11] = "";
            static int s_CachedYmd = 0; // year * 10000 + month * 100 + day

            std::tm timeinfo{};
            if (!GetLocalTm(timeinfo))
            {
                if (s_Date[0] == '\0')
                {
                    // Best-effort fallback so callers always get a non-null C string.
                    s_Date[0] = '0'; s_Date[1] = '1'; s_Date[2] = '-';
                    s_Date[3] = '0'; s_Date[4] = '1'; s_Date[5] = '-';
                    s_Date[6] = '1'; s_Date[7] = '9'; s_Date[8] = '7'; s_Date[9] = '0';
                    s_Date[10] = '\0';
                }
                return s_Date;
            }

            const int month = timeinfo.tm_mon + 1;
            const int day = timeinfo.tm_mday;
            const int year = timeinfo.tm_year + 1900;
            const int ymd = ToSortableYmd(month, day, year);

            if (ymd != s_CachedYmd || s_Date[0] == '\0')
            {
                s_CachedYmd = ymd;
                std::snprintf(s_Date, sizeof(s_Date), "%02d-%02d-%04d", month, day, year);
            }

            return s_Date;
        }

        std::string GetDateTime()
        {
            // Keep string API for create/join project paths; hot UI uses GetTodayMmDdYyyy().
            char buf[11];
            if (!GetDateMmDdYyyy(buf, sizeof(buf)))
                return {};
            return std::string(buf);
        }

        std::string GetSubmissionTimestamp()
        {
            std::tm timeinfo{};
            if (!GetLocalTm(timeinfo))
                return {};

            char buf[20];
            // YYYY-MM-DD HH:MM:SS = 19 chars + NUL
            const int written = std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            if (written != 19)
                return {};
            return std::string(buf);
        }

        bool IsValidMmDdYyyy(const char* s)
        {
            int m = 0, d = 0, y = 0;
            return ParseMmDdYyyy(s, m, d, y);
        }

        bool IsValidMmDdYyyy(const std::string& s)
        {
            return IsValidMmDdYyyy(s.c_str());
        }

        int CompareMmDdYyyy(const char* a, const char* b)
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

        int CompareMmDdYyyy(const std::string& a, const std::string& b)
        {
            return CompareMmDdYyyy(a.c_str(), b.c_str());
        }

        std::string ComputeMilestoneStatus(float progressPercentage,
            const std::string& startDateMmDdYyyy, const std::string& endDateMmDdYyyy)
        {
            if (progressPercentage >= 100.0f)
                return "completed";

            const char* today = GetTodayMmDdYyyy();

            // Before the planned window -> not started.
            if (CompareMmDdYyyy(today, startDateMmDdYyyy.c_str()) < 0)
                return "not started";

            // Within [start, end] inclusive -> in progress.
            if (CompareMmDdYyyy(today, endDateMmDdYyyy.c_str()) <= 0)
                return "in progress";

            // Past end date but not 100% complete -> still treated as in progress.
            return "in progress";
        }
    }
}
