#pragma once

#include <string>

/**
 * Directory containing the scene_viewer binary (typically .../driver_test/inner).
 * Resolved once from /proc/self/exe on Linux.
 */
std::string innerDirectory();

/**
 * Shared texture root: innerDirectory() + "/../textures" → .../driver_test/textures
 * Override with environment variable TEXTURES_PATH.
 */
std::string texturesPath();

/** Same directory as texturesPath(); name matches scene.h documentation. */
inline std::string TEXTURES_PATH()
{
    return texturesPath();
}

/** default.scene next to the binary: innerDirectory() + "/default.scene" */
std::string defaultSceneFilePath();
