#pragma once

#include "Log.h"

#include <cstdlib>

// Platform debugger break. Falls back to std::abort() when unavailable.
#if defined(_MSC_VER)
    #include <intrin.h>
    #define TT_DEBUGBREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
    #if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
        #define TT_DEBUGBREAK() __builtin_debugtrap()
    #else
        #include <csignal>
        #define TT_DEBUGBREAK() std::raise(SIGTRAP)
    #endif
#else
    #define TT_DEBUGBREAK() std::abort()
#endif

// Asserts are active in debug builds (or when TT_ENABLE_ASSERTS is forced on).
#if !defined(TT_ENABLE_ASSERTS)
    #if defined(_DEBUG) || !defined(NDEBUG)
        #define TT_ENABLE_ASSERTS 1
    #else
        #define TT_ENABLE_ASSERTS 0
    #endif
#endif

namespace TrackingTool::Detail
{
    // Logs a failed check at Fatal severity with full source context.
    template<typename... Args>
    inline void ReportAssertionFailure(const char* expr, const char* file, int line,
        std::format_string<Args...> fmt, Args&&... args)
    {
        Log::Fatal("Assertion failed: ({}) at {}:{}", expr, file, line);
        Log::Fatal(fmt, std::forward<Args>(args)...);
    }

    inline void ReportAssertionFailure(const char* expr, const char* file, int line)
    {
        Log::Fatal("Assertion failed: ({}) at {}:{}", expr, file, line);
    }
}

// Internal helper: evaluate condition, report + break on failure.
#define TT_INTERNAL_CHECK(condition, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(condition))                                                              \
        {                                                                              \
            ::TrackingTool::Detail::ReportAssertionFailure(                            \
                #condition, __FILE__, __LINE__, ## __VA_ARGS__);                       \
            TT_DEBUGBREAK();                                                           \
        }                                                                              \
    } while (false)

// TT_ASSERT: checks an invariant. Compiled out entirely in release builds.
// Optional trailing args form a std::format message, e.g.
//     TT_ASSERT(ptr, "expected non-null {}", name);
#if TT_ENABLE_ASSERTS
    #define TT_ASSERT(condition, ...) TT_INTERNAL_CHECK(condition, __VA_ARGS__)
#else
    #define TT_ASSERT(condition, ...) ((void)0)
#endif

// TT_VERIFY: like TT_ASSERT but the condition is always evaluated, even in
// release builds (use when the expression has side effects you must keep).
// In release builds the failure is logged but execution continues.
#if TT_ENABLE_ASSERTS
    #define TT_VERIFY(condition, ...) TT_INTERNAL_CHECK(condition, __VA_ARGS__)
#else
    #define TT_VERIFY(condition, ...)                                                  \
        do                                                                             \
        {                                                                              \
            if (!(condition))                                                          \
                ::TrackingTool::Detail::ReportAssertionFailure(                        \
                    #condition, __FILE__, __LINE__, ## __VA_ARGS__);                   \
        } while (false)
#endif
