#pragma once

#include <string>

namespace TrackingTool
{
    namespace Utils
    {
        // Local date as MM-DD-YYYY (matches project date display/input convention).
        std::string GetDateTime();

        // True when s is a valid calendar date in MM-DD-YYYY form (leading zeros optional for month/day).
        bool IsValidMmDdYyyy(const std::string& s);

        // Lexicographic calendar compare of two valid MM-DD-YYYY strings.
        // Returns -1 if a < b, 0 if equal, 1 if a > b. Returns 0 if either is invalid.
        int CompareMmDdYyyy(const std::string& a, const std::string& b);

        // Milestone status from progress and date window:
        // - completed: progress >= 100
        // - not started: today before start date
        // - in progress: today on/after start (including within or past end date while incomplete)
        std::string ComputeMilestoneStatus(float progressPercentage,
            const std::string& startDateMmDdYyyy, const std::string& endDateMmDdYyyy);
    }
}
