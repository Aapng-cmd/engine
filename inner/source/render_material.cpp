#include "render_material.h"
#include "vector.h"

#include <algorithm>
#include <cmath>

AlphaReflect decomposeAlphaReflect(double alpha)
{
    AlphaReflect ar;
    if (alpha <= 1.0) {
        ar.opacity = std::clamp(alpha, 0.0, 1.0);
        ar.reflect = 0.0;
    } else {
        ar.opacity = 1.0;
        ar.reflect = std::clamp(alpha - 1.0, 0.0, 1.0);
    }
    return ar;
}

void initMatteSceneLighting()
{
    glDisable(GL_COLOR_MATERIAL);
    const GLfloat zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
    const GLfloat lightAmb[] = {0.28f, 0.28f, 0.30f, 1.0f};
    const GLfloat lightDiff[] = {0.82f, 0.82f, 0.86f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmb);
    resetFigureMaterial();
}

void applyFigureMaterial(double opacity, double reflect, const vec<>* surfaceColor)
{
    glDisable(GL_COLOR_MATERIAL);
    const float o = static_cast<float>(std::clamp(opacity, 0.0, 1.0));
    const float r = static_cast<float>(std::clamp(reflect, 0.0, 1.0));
    const float spec = r * 0.85f;
    const float shin = r > 0.01f ? (8.0f + r * 88.0f) : 1.0f;
    const GLfloat zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
    float cr = 1.0f, cg = 1.0f, cb = 1.0f;
    if (surfaceColor) {
        cr = static_cast<float>(std::clamp(surfaceColor->x, 0.0, 1.0));
        cg = static_cast<float>(std::clamp(surfaceColor->y, 0.0, 1.0));
        cb = static_cast<float>(std::clamp(surfaceColor->z, 0.0, 1.0));
    }
    const GLfloat amb[] = {0.22f * o * cr, 0.22f * o * cg, 0.24f * o * cb, o};
    const GLfloat diff[] = {0.78f * o * cr, 0.78f * o * cg, 0.82f * o * cb, o};
    const GLfloat spc[] = {spec, spec, spec, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spc);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shin);
}

void resetFigureMaterial()
{
    const GLfloat zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
    const GLfloat amb[] = {0.22f, 0.22f, 0.24f, 1.0f};
    const GLfloat diff[] = {0.78f, 0.78f, 0.82f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 1.0f);
}

void bindTextureReflective(GLuint tex, double reflect, bool isWaterLike)
{
    if (tex == 0)
        return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (isWaterLike || reflect > 0.01) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x8370);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x8370);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}
