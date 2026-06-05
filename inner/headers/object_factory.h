#pragma once

#include "collision_repr.h"
#include "figures.h"
#include <string>
#include <vector>

/** Builds scene objects (primitives + figures from figures.h). Extra layout is per-type; see scene editor. */
based* createSceneObject(const std::string& type, double px, double py, double pz, double sx, double sy, double sz,
                         double rx, double ry, double rz, const std::vector<double>& extra, GLuint tex,
                         std::string* err = nullptr);

/** Expected number of extra doubles after tex index (for validation). -1 = variable / checked in factory. */
int expectedExtraCount(const std::string& type);

/** Multi-part figures (wrapped presets); mass is computed from parts only. */
bool isComplexFigureType(const std::string& type);
bool shapeUsesTriangleCollision(const based* obj);
