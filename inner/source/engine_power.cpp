#include "engine_power.h"
#include "render_settings.h"

#include <algorithm>
#include <cmath>

namespace engine {

static int gLevel = 2;

static double tFrom2()
{
    return (static_cast<double>(gLevel) - 2.0) / 8.0;
}

void setPowerLevel(int level)
{
    gLevel = std::clamp(level, 1, 10);
    rs::setLodQuality(lodQuality());
}

int powerLevel()
{
    return gLevel;
}

double lodQuality()
{
    /* 2 → 1.0 (current), 1 → ~0.62, 10 → ~3.2 */
    return std::clamp(1.0 + tFrom2() * 2.2, 0.45, 4.0);
}

int physicsSubsteps()
{
    return std::clamp(static_cast<int>(std::lround(4.0 + tFrom2() * 8.0)), 2, 12);
}

int maxCollisionSubdiv()
{
    return std::clamp(static_cast<int>(std::lround(24.0 + tFrom2() * 40.0)), 8, 64);
}

int maxMeshVerts()
{
    return std::clamp(static_cast<int>(std::lround(256.0 * std::pow(2.0, tFrom2() * 4.0))), 96, 4096);
}

int maxMeshTris()
{
    return std::clamp(static_cast<int>(std::lround(512.0 * std::pow(2.0, tFrom2() * 4.0))), 192, 8192);
}

int maxTessSlices()
{
    return std::clamp(static_cast<int>(std::lround(72.0 * (1.0 + tFrom2() * 1.7))), 24, 192);
}

int maxTessStacks()
{
    return std::clamp(static_cast<int>(std::lround(48.0 * (1.0 + tFrom2() * 1.7))), 16, 128);
}

int maxFaceSubdiv()
{
    return std::clamp(static_cast<int>(std::lround(20.0 + tFrom2() * 28.0)), 8, 48);
}

int hypersphereSlices()
{
    return std::clamp(static_cast<int>(std::lround(8.0 + tFrom2() * 16.0)), 6, 32);
}

int hypersphereStacks()
{
    return std::clamp(static_cast<int>(std::lround(6.0 + tFrom2() * 12.0)), 4, 24);
}

int multisampleHint()
{
    if (gLevel >= 8)
        return 8;
    if (gLevel >= 5)
        return 4;
    return 0;
}

int defaultWindowWidth()
{
    if (gLevel >= 8)
        return 1920;
    if (gLevel >= 5)
        return 1280;
    return 800;
}

int defaultWindowHeight()
{
    if (gLevel >= 8)
        return 1080;
    if (gLevel >= 5)
        return 720;
    return 600;
}

const char* powerLabel()
{
    switch (gLevel) {
    case 1:
        return "1 (very low)";
    case 2:
        return "2 (current / low-power)";
    case 3:
        return "3";
    case 4:
        return "4";
    case 5:
        return "5 (mid)";
    case 6:
        return "6";
    case 7:
        return "7";
    case 8:
        return "8 (high)";
    case 9:
        return "9";
    default:
        return "10 (32GB / RTX 4090-class)";
    }
}

} // namespace engine
