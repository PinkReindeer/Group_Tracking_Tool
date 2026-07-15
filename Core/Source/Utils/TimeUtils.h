#pragma once

#include <cstddef>
#include <string>

namespace TrackingTool
{
    namespace Utils
    {
        // Local date as MM-DD-YYYY (matches project date display/input convention).
        // Allocates a new std::string — prefer GetTodayMmDdYyyy() on hot paths.
        std::string GetDateTime();

        // Writes local date as MM-DD-YYYY into outBuf (needs at least 11 bytes including NUL).
        // Returns false if outBuf is null or outBufSize < 11.
        bool GetDateMmDdYyyy(char* outBuf, size_t outBufSize);

        // Cached local date as MM-DD-YYYY. No heap allocation.
        // Refreshes only when the calendar day changes (safe for per-frame UI checks).
        // Pointer is valid for the process lifetime; do not free.
        const char* GetTodayMmDdYyyy();

        // Local timestamp for deliverable submission: "YYYY-MM-DD HH:MM:SS" (PostgreSQL-friendly).
        std::string GetSubmissionTimestamp();

        // True when s is a valid calendar date in MM-DD-YYYY form (leading zeros optional for month/day).
        bool IsValidMmDdYyyy(const std::string& s);
        bool IsValidMmDdYyyy(const char* s);

        // Lexicographic calendar compare of two valid MM-DD-YYYY strings.
        // Returns -1 if a < b, 0 if equal, 1 if a > b. Returns 0 if either is invalid.
        int CompareMmDdYyyy(const std::string& a, const std::string& b);
        int CompareMmDdYyyy(const char* a, const char* b);

        // Milestone status from progress and date window:
        // - completed: progress >= 100
        // - not started: today before start date
        // - in progress: today on/after start (including within or past end date while incomplete)
        std::string ComputeMilestoneStatus(float progressPercentage,
            const std::string& startDateMmDdYyyy, const std::string& endDateMmDdYyyy);
    }
}
