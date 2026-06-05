#pragma once

#include "scene.h"
#include <string>

/** Clear user objects (frees pointers), then load from editor scene file. Returns false on parse error. */
bool loadEditorSceneFile(Scene& scene, const std::string& path);
