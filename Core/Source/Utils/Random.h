#pragma once

#include <random>

class Random
{
public:
    static void Init()
    {
        static bool seeded = false;
        if (seeded) return;
        s_RandomEngine.seed(std::random_device()());
        seeded = true;
    }

    static uint32_t UInt()
    {
        return s_Distribution(s_RandomEngine);
    }

    static uint32_t UInt(uint32_t max)
    {
        return s_Distribution(s_RandomEngine) % (max + 1);
    }

    static int Int(uint32_t max)
    {
        return (int)UInt(max);
    }

    static int IntSymmetric(int n)
    {
        return (int)UInt(n * 2) - n;
    }

    static uint32_t UInt(uint32_t min, uint32_t max)
    {
        return min + (s_Distribution(s_RandomEngine) % (max - min + 1));
    }

    static float Float()
    {
        return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint32_t>::max();
    }

    static float FloatSigned()
    {
        return Float() * 2.0f - 1.0f;
    }

private:
    inline static std::mt19937 s_RandomEngine;
    inline static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
};