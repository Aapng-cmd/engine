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

void applySurfacePassState(double opacity, double reflect)
{
    const float r = static_cast<float>(std::clamp(reflect, 0.0, 1.0));
    const bool glassy = opacity < 0.999 || r > 0.01f;
    /*
     * Sphere-map / texgen is a texture-object + enable bit leftover: one reflective
     * body (or an old wrap hack) made later objects look mirrored even at opacity=1.
     */
    if (r > 0.01f) {
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    } else {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);
    }
    if (glassy) {
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        /* Solid: do not let PNG alpha punch a hole to the sky (reads as a reflection). */
        glDisable(GL_BLEND);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.05f);
    }
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

void bindTextureReflective(GLuint tex, double reflect)
{
    if (tex == 0)
        return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    /* Keep REPEAT so a shared texture is not permanently clamped after a ground pass. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (reflect > 0.01) {
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    } else {
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }
}
