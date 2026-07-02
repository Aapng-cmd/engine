#pragma once

#include "vector.h"
#include <GL/gl.h>

/** Разложение alpha: [0,1] — прозрачность, (1,2] — сила отражения. */
struct AlphaReflect {
    double opacity = 1.0;
    double reflect = 0.0;
};

AlphaReflect decomposeAlphaReflect(double alpha);

void initMatteSceneLighting();
void applyFigureMaterial(double opacity, double reflect, const vec<>* surfaceColor = nullptr);
void resetFigureMaterial();
void bindTextureReflective(GLuint tex, double reflect, bool isWaterLike);
