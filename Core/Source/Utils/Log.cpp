#include "Log.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>

namespace TrackingTool
{
    namespace
    {
#if defined(_DEBUG) || !defined(NDEBUG)
        std::atomic<LogLevel> s_Level{ LogLevel::Trace };
#else
        std::atomic<LogLevel> s_Level{ LogLevel::Info };
#endif
        std::atomic<bool> s_ColorEnabled{ true };

        // Serializes writes so log lines from multiple threads don't interleave.
        std::mutex s_Mutex;

        const char* LevelTag(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::Trace: return "TRACE";
                case LogLevel::Info:  return "INFO ";
                case LogLevel::Warn:  return "WARN ";
                case LogLevel::Error: return "ERROR";
                case LogLevel::Fatal: return "FATAL";
                default:              return "?????";
            }
        }

        // ANSI foreground colors; empty when color is disabled.
        const char* LevelColor(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::Trace: return "\033[37m";   // gray
                case LogLevel::Info:  return "\033[32m";   // green
                case LogLevel::Warn:  return "\033[33m";   // yellow
                case LogLevel::Error: return "\033[31m";   // red
                case LogLevel::Fatal: return "\033[1;41m"; // bold on red bg
                default:              return "";
            }
        }

        constexpr const char* k_ColorReset = "\033[0m";

        std::string Timestamp()
        {
            using namespace std::chrono;
            const auto now = system_clock::now();
            const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            return std::format("{:%H:%M:%S}.{:03}",
                std::chrono::floor<seconds>(now), ms.count());
        }
    }

    void Log::SetLevel(LogLevel level)
    {
        s_Level.store(level, std::memory_order_relaxed);
    }

    LogLevel Log::GetLevel()
    {
        return s_Level.load(std::memory_order_relaxed);
    }

    void Log::SetColorEnabled(bool enabled)
    {
        s_ColorEnabled.store(enabled, std::memory_order_relaxed);
    }

    void Log::Write(LogLevel level, std::string_view message)
    {
        if (level < GetLevel())
            return;

        // Warnings and above go to stderr so they survive stdout redirection.
        std::ostream& out = (level >= LogLevel::Warn) ? std::cerr : std::cout;

        const bool color = s_ColorEnabled.load(std::memory_order_relaxed);

        std::lock_guard<std::mutex> lock(s_Mutex);
        if (color)
            out << LevelColor(level);

        out << '[' << Timestamp() << "] [" << LevelTag(level) << "] " << message;

        if (color)
            out << k_ColorReset;

        out << '\n';

        // Make sure high-severity messages are visible immediately, even on crash.
        if (level >= LogLevel::Error)
            out.flush();
    }
}
