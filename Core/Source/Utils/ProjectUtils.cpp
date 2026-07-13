#include "ProjectUtils.h"

#include "Utils/Random.h"

namespace TrackingTool
{
    namespace Utils
    {
        std::string GenerateProjectCode()
        {
            const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            std::string code;
            code.reserve(6);

            Random::Init();
            for (int i = 0; i < 6; ++i)
            {
                code += chars[Random::UInt(static_cast<uint32_t>(chars.size() - 1))];
            }
            return code;
        }
    }
}
