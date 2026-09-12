#pragma once

#include <string>

/**
 * Directory containing the scene_viewer binary (typically .../driver_test/inner).
 * Resolved from /proc/self/exe, DRIVER_TEST_ROOT, or setInnerDirectoryOverride().
 */
std::string innerDirectory();

/** Force inner/ (used by the Qt editor, whose binary lives under outer/). */
void setInnerDirectoryOverride(const std::string& absInner);

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

/** Сцена для ручной проверки коллизий всех базовых фигур. */
std::string defaultCollisionTestScenePath();
