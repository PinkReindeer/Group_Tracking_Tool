#pragma once

#include <format>
#include <string_view>
#include <utility>

namespace TrackingTool
{
    // Severity levels, ordered from most to least verbose. Off disables all output.
    enum class LogLevel
    {
        Trace = 0,
        Info,
        Warn,
        Error,
        Fatal,
        Off
    };

    // Lightweight, thread-safe, std::format-based logger for the Core framework.
    //
    // Usage:
    //     Log::Info("Window created: {}x{}", width, height);
    //     Log::Error("Failed to load '{}': {}", path, reason);
    //
    // Output goes to stdout for Trace/Info and stderr for Warn/Error/Fatal,
    // each line prefixed with a timestamp and the level tag, optionally colored.
    class Log
    {
    public:
        // Minimum level to emit. Messages below this are dropped cheaply
        // (formatting is skipped entirely). Defaults to Trace in debug builds
        // and Info in release builds.
        static void SetLevel(LogLevel level);
        static LogLevel GetLevel();

        // Toggle ANSI color escape codes in console output (on by default).
        static void SetColorEnabled(bool enabled);

        // Core sink: emit an already-formatted message at the given level.
        // Prefer the typed helpers below; this exists for non-formatted strings.
        static void Write(LogLevel level, std::string_view message);

        template<typename... Args>
        static void Trace(std::format_string<Args...> fmt, Args&&... args)
        {
            Dispatch(LogLevel::Trace, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Info(std::format_string<Args...> fmt, Args&&... args)
        {
            Dispatch(LogLevel::Info, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Warn(std::format_string<Args...> fmt, Args&&... args)
        {
            Dispatch(LogLevel::Warn, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Error(std::format_string<Args...> fmt, Args&&... args)
        {
            Dispatch(LogLevel::Error, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Fatal(std::format_string<Args...> fmt, Args&&... args)
        {
            Dispatch(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
        }

    private:
        template<typename... Args>
        static void Dispatch(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
        {
            // Cheap early-out so disabled levels cost nothing beyond the comparison.
            if (level < GetLevel())
                return;

            Write(level, std::format(fmt, std::forward<Args>(args)...));
        }
    };
}
