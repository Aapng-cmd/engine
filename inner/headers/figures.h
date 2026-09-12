#pragma once

#include "render_material.h"
#include "render_settings.h"
#include "vector.h"
#include <GL/glut.h>
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline double sign(double x) { return (x > 0) ? 1.0 : ((x < 0) ? -1.0 : 0.0); }

inline vec<> rotateZ(const vec<>& v, double angle)
{
    const double c = cos(angle), s = sin(angle);
    return vec<>(c * v.x - s * v.y, s * v.x + c * v.y, v.z);
}

inline vec<> rotateY(const vec<>& v, double angle)
{
    const double c = cos(angle), s = sin(angle);
    return vec<>(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

inline vec<> rotateX(const vec<>& v, double angle)
{
    const double c = cos(angle), s = sin(angle);
    return vec<>(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
}

inline void mergeSpheres(const vec<>& c1, double r1, const vec<>& c2, double r2, vec<>& out_c, double& out_r)
{
    const double d = (c1 - c2).len();
    if (d + r2 <= r1) {
        out_c = c1;
        out_r = r1;
    } else if (d + r1 <= r2) {
        out_c = c2;
        out_r = r2;
    } else {
        out_r = (d + r1 + r2) * 0.5;
        out_c = c1 + (c2 - c1) * (out_r - r1) / std::max(1e-9, d);
    }
}

/**
 * Unit cube [-0.5,0.5]^3 with per-face UV and normals.
 * glutSolidCube emits no texture coordinates, so a bound texture would collapse to one texel.
 */
inline void drawUnitCubeTextured(const vec<>& rep = vec<>(1, 1, 1))
{
    const float h = 0.5f;
    const float rx = static_cast<float>(rep.x);
    const float ry = static_cast<float>(rep.y);
    const float rz = static_cast<float>(rep.z);
    /* Все грани обходятся против часовой стрелки снаружи, иначе их срежет GL_CULL_FACE. */
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-h, -h, h);
    glTexCoord2f(rx, 0); glVertex3f(h, -h, h);
    glTexCoord2f(rx, ry); glVertex3f(h, h, h);
    glTexCoord2f(0, ry); glVertex3f(-h, h, h);
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(h, -h, -h);
    glTexCoord2f(rx, 0); glVertex3f(-h, -h, -h);
    glTexCoord2f(rx, ry); glVertex3f(-h, h, -h);
    glTexCoord2f(0, ry); glVertex3f(h, h, -h);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-h, h, h);
    glTexCoord2f(rx, 0); glVertex3f(h, h, h);
    glTexCoord2f(rx, rz); glVertex3f(h, h, -h);
    glTexCoord2f(0, rz); glVertex3f(-h, h, -h);
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-h, -h, -h);
    glTexCoord2f(rx, 0); glVertex3f(h, -h, -h);
    glTexCoord2f(rx, rz); glVertex3f(h, -h, h);
    glTexCoord2f(0, rz); glVertex3f(-h, -h, h);
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(h, -h, h);
    glTexCoord2f(rz, 0); glVertex3f(h, -h, -h);
    glTexCoord2f(rz, ry); glVertex3f(h, h, -h);
    glTexCoord2f(0, ry); glVertex3f(h, h, h);
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-h, -h, -h);
    glTexCoord2f(rz, 0); glVertex3f(-h, -h, h);
    glTexCoord2f(rz, ry); glVertex3f(-h, h, h);
    glTexCoord2f(0, ry); glVertex3f(-h, h, -h);
    glEnd();
}

/**
 * Torus with the same orientation as glutSolidTorus(tube, major, sides, rings):
 * ring in the XY plane, tube along Z. Unlike GLUT it emits UV coordinates.
 */
inline void drawTorusTextured(double tube, double major, int sides, int rings, const vec<>& rep = vec<>(1, 1, 1))
{
    const double r = std::abs(tube);
    const double R = std::abs(major);
    sides = std::max(3, sides);
    rings = std::max(3, rings);
    for (int i = 0; i < rings; ++i) {
        const double u0 = 2.0 * M_PI * static_cast<double>(i) / rings;
        const double u1 = 2.0 * M_PI * static_cast<double>(i + 1) / rings;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= sides; ++j) {
            const double v = 2.0 * M_PI * static_cast<double>(j) / sides;
            const double cv = std::cos(v), sv = std::sin(v);
            for (int k = 0; k < 2; ++k) {
                const double u = k == 0 ? u0 : u1;
                const double cu = std::cos(u), su = std::sin(u);
                const double w = R + r * cv;
                glNormal3d(cv * cu, cv * su, sv);
                glTexCoord2d(rep.x * (k == 0 ? i : i + 1) / static_cast<double>(rings),
                             rep.y * j / static_cast<double>(sides));
                glVertex3d(w * cu, w * su, r * sv);
            }
        }
        glEnd();
    }
}

/**
 * Cone matching appendConeTriangles: centred on origin, apex +Y, base y=-h/2.
 * UV: U around the base, V from base (0) to apex (1). Same triangles as the collision mesh,
 * so the texture sits on the wire overlay instead of a shifted gluCylinder.
 */
inline void drawConeTextured(double radius, double height, int segments, const vec<>& rep = vec<>(1, 1, 1))
{
    const double r = std::abs(radius);
    const double h = std::abs(height);
    segments = std::max(6, segments);
    const double hb = h * 0.5;
    const vec<> apex(0, hb, 0);
    const double yb = -hb;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < segments; ++i) {
        const double a0 = 2.0 * M_PI * static_cast<double>(i) / segments;
        const double a1 = 2.0 * M_PI * static_cast<double>(i + 1) / segments;
        const vec<> p0(r * std::cos(a0), yb, r * std::sin(a0));
        const vec<> p1(r * std::cos(a1), yb, r * std::sin(a1));
        /* Outward normal of the side triangle (same winding as collision: p1, p0, apex). */
        const vec<> n = !((p0 - p1) ^ (apex - p1));
        const double u0 = rep.x * static_cast<double>(i) / segments;
        const double u1 = rep.x * static_cast<double>(i + 1) / segments;
        const double um = 0.5 * (u0 + u1);
        glNormal3d(n.x, n.y, n.z);
        glTexCoord2d(u1, 0); glVertex3d(p1.x, p1.y, p1.z);
        glTexCoord2d(u0, 0); glVertex3d(p0.x, p0.y, p0.z);
        glTexCoord2d(um, rep.y); glVertex3d(apex.x, apex.y, apex.z);
        /* Base cap, facing −Y. */
        glNormal3d(0, -1, 0);
        glTexCoord2d(0.5 * rep.x, 0.5 * rep.y);
        glVertex3d(0, yb, 0);
        glTexCoord2d((0.5 + 0.5 * std::cos(a1)) * rep.x, (0.5 + 0.5 * std::sin(a1)) * rep.y);
        glVertex3d(p1.x, p1.y, p1.z);
        glTexCoord2d((0.5 + 0.5 * std::cos(a0)) * rep.x, (0.5 + 0.5 * std::sin(a0)) * rep.y);
        glVertex3d(p0.x, p0.y, p0.z);
    }
    glEnd();
}

/** Square pyramid centred on its bounding box, apex +Y, with UV and per-face normals. */
inline void drawPyramidTextured(double base, double height)
{
    const double h = std::abs(height);
    const double a = std::abs(base) * 0.5;
    const double hb = h * 0.5;
    const vec<> apex(0, hb, 0);
    const vec<> corner[4] = {vec<>(-a, -hb, -a), vec<>(a, -hb, -a), vec<>(a, -hb, a), vec<>(-a, -hb, a)};
    /* Обход против часовой стрелки снаружи: иначе грани уйдут под GL_CULL_FACE. */
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 4; ++i) {
        const vec<>& p0 = corner[i];
        const vec<>& p1 = corner[(i + 1) % 4];
        const vec<> n = !((p0 - p1) ^ (apex - p1));
        glNormal3d(n.x, n.y, n.z);
        glTexCoord2d(0, 0); glVertex3d(p1.x, p1.y, p1.z);
        glTexCoord2d(1, 0); glVertex3d(p0.x, p0.y, p0.z);
        glTexCoord2d(0.5, 1); glVertex3d(apex.x, apex.y, apex.z);
    }
    glNormal3d(0, -1, 0);
    glTexCoord2d(0, 0); glVertex3d(corner[0].x, corner[0].y, corner[0].z);
    glTexCoord2d(1, 0); glVertex3d(corner[1].x, corner[1].y, corner[1].z);
    glTexCoord2d(1, 1); glVertex3d(corner[2].x, corner[2].y, corner[2].z);
    glTexCoord2d(0, 0); glVertex3d(corner[0].x, corner[0].y, corner[0].z);
    glTexCoord2d(1, 1); glVertex3d(corner[2].x, corner[2].y, corner[2].z);
    glTexCoord2d(0, 1); glVertex3d(corner[3].x, corner[3].y, corner[3].z);
    glEnd();
}

namespace physmath {
inline double sphereVolume(double r) { return (4.0 / 3.0) * M_PI * r * r * r; }
inline double boxVolume(double x, double y, double z) { return x * y * z; }
inline double cylinderVolume(double r, double h) { return M_PI * r * r * h; }
inline double coneVolume(double r, double h) { return (M_PI * r * r * h) / 3.0; }
} // namespace physmath

/**
 * Сколько раз текстура повторяется по каждой оси.
 * Без этого текстура растягивается на всю фигуру: у плиты 60×60 виден один
 * увеличенный пиксель, что неотличимо от заливки цветом.
 */
inline vec<> autoTexRepeat(double sizeX, double sizeY, double sizeZ, double unitsPerTile = 8.0)
{
    const double u = std::max(0.001, unitsPerTile);
    return vec<>(std::max(1.0, std::abs(sizeX) / u), std::max(1.0, std::abs(sizeY) / u),
                 std::max(1.0, std::abs(sizeZ) / u));
}

struct based {
    double renderAlpha = 1.0;
    /** [0,1] отражение (из alpha (1,2] в PHYS). */
    double reflectAmount = 0.0;
    GLuint textureID = 0;
    /** Повторов текстуры по осям (см. autoTexRepeat). */
    vec<> texRepeat = vec<>(1, 1, 1);
    virtual ~based() = default;
    virtual void Draw(double t) {}
    virtual void AddChild(based* /*p*/) {}
    virtual void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) = 0;
    virtual void emergency_bounding_sphere_calc_protocol(vec<>& center, double& radius, double t)
    {
        std::vector<std::pair<vec<>, double>> parts;
        getBoundingSpheres(parts, t);
        if (parts.empty()) {
            center = vec<>(0, 0, 0);
            radius = 0.0;
            return;
        }
        center = parts[0].first;
        radius = parts[0].second;
        for (size_t i = 1; i < parts.size(); ++i)
            mergeSpheres(center, radius, parts[i].first, parts[i].second, center, radius);
    }
    void setTexture(GLuint texID) { textureID = texID; }
};

/** Shared solid-color / textured draw helpers for basic shapes at local origin. */
struct SolidSphere : public based {
    double radius = 1;
    vec<> color = vec<>(0.75, 0.75, 0.75);
    static GLUquadric* quad;

    SolidSphere(double radius = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0)
        : radius(radius), color(color)
    {
        textureID = tex;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
            gluQuadricNormals(quad, GLU_SMOOTH);
        }
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        out.push_back({vec<>(0, 0, 0), std::abs(radius)});
    }

    void Draw(double /*t*/) override
    {
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
            gluSphere(quad, std::abs(radius), rs::ed_sph_slc, rs::ed_sph_stk);
            glDisable(GL_TEXTURE_2D);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
            glutSolidSphere(std::abs(radius), rs::ed_sph_slc, rs::ed_sph_stk);
        }
    }
};

struct SolidCube : public based {
    double hx = 0.5, hy = 0.5, hz = 0.5;
    vec<> color = vec<>(0.75, 0.75, 0.75);

    SolidCube(double sx = 1, double sy = 1, double sz = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0)
        : hx(0.5 * sx), hy(0.5 * sy), hz(0.5 * sz), color(color)
    {
        textureID = tex;
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        out.push_back({vec<>(0, 0, 0), std::sqrt(hx * hx + hy * hy + hz * hz)});
    }

    void drawLocal(double /*t*/)
    {
        glPushMatrix();
        glScaled(2 * hx, 2 * hy, 2 * hz);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
        }
        drawUnitCubeTextured(texRepeat);
        if (textureID != 0)
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    void Draw(double t) override { drawLocal(t); }
};

struct SolidCylinder : public based {
    double radius = 0.5;
    double height = 1;
    vec<> color = vec<>(0.75, 0.75, 0.75);
    static GLUquadric* quad;

    SolidCylinder(double radius = 0.5, double height = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0)
        : radius(radius), height(height), color(color)
    {
        textureID = tex;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
            gluQuadricNormals(quad, GLU_SMOOTH);
        }
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        const double hh = 0.5 * std::abs(height);
        const double r = std::abs(radius);
        out.push_back({vec<>(0, 0, 0), std::sqrt(r * r + hh * hh)});
    }

    void Draw(double /*t*/) override
    {
        glPushMatrix();
        glRotated(-90, 1, 0, 0);
        glTranslated(0, 0, -0.5 * height);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
            gluCylinder(quad, radius, radius, height, rs::ed_cyl_slc, 1);
            glDisable(GL_TEXTURE_2D);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
            gluCylinder(quad, radius, radius, height, rs::ed_cyl_slc, 1);
        }
        glPopMatrix();
    }
};

struct SolidCone : public based {
    double radius = 0.5;
    double height = 1;
    vec<> color = vec<>(0.75, 0.75, 0.75);
    static GLUquadric* quad;

    SolidCone(double radius = 0.5, double height = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0)
        : radius(radius), height(height), color(color)
    {
        textureID = tex;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
            gluQuadricNormals(quad, GLU_SMOOTH);
        }
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        const double r = std::abs(radius);
        const double h = std::abs(height);
        out.push_back({vec<>(0, 0, h * 0.5), std::sqrt(r * r + (h * 0.5) * (h * 0.5))});
    }

    void Draw(double /*t*/) override
    {
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
        }
        drawConeTextured(radius, height, rs::cone_seg, texRepeat);
        if (textureID != 0)
            glDisable(GL_TEXTURE_2D);
    }
};

struct SolidPyramid : public based {
    double base = 1;
    double height = 1;
    vec<> color = vec<>(0.75, 0.75, 0.75);

    SolidPyramid(double base = 1, double height = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0)
        : base(base), height(height), color(color)
    {
        textureID = tex;
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        const double h = std::abs(height);
        const double a = std::abs(base) * 0.5;
        out.push_back({vec<>(0, h * 0.5, 0), std::sqrt(a * a + (h * 0.5) * (h * 0.5))});
    }

    void Draw(double /*t*/) override
    {
        const double h = std::abs(height);
        const double a = std::abs(base) * 0.5;
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
        }
        drawPyramidTextured(2.0 * a, h);
        if (textureID != 0)
            glDisable(GL_TEXTURE_2D);
    }
};

struct SolidTorus : public based {
    double innerR = 0.3;
    double outerR = 1;
    vec<> color = vec<>(0.75, 0.75, 0.75);

    SolidTorus(double innerR = 0.3, double outerR = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0)
        : innerR(innerR), outerR(outerR), color(color)
    {
        textureID = tex;
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        out.push_back({vec<>(0, 0, 0), (std::abs(innerR) + std::abs(outerR)) * 0.5});
    }

    void Draw(double /*t*/) override
    {
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
        }
        drawTorusTextured(innerR, outerR, rs::ed_tor_s, rs::ed_tor_r, texRepeat);
        if (textureID != 0)
            glDisable(GL_TEXTURE_2D);
    }
};

class GroundPlane : public based {
    GLuint textureID;
    int edgeLength1 = 200;
    int edgeLength2 = 200;
    double reflectStrength = 0.0;

public:
    GroundPlane(GLuint texID, int e1 = 200, int e2 = 200) : textureID(texID), edgeLength1(e1), edgeLength2(e2) {}

    void setReflect(double strength)
    {
        reflectStrength = std::clamp(strength, 0.0, 1.0);
        reflectAmount = reflectStrength;
    }

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        out.push_back({vec<>(0, 0, 0),
                       std::sqrt(static_cast<double>(edgeLength1 * edgeLength1 + edgeLength2 * edgeLength2))});
    }

    void drawQuadUnlit() const
    {
        const float uRep = static_cast<float>(edgeLength1) / 25.0f;
        const float vRep = static_cast<float>(edgeLength2) / 25.0f;
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);
        glVertex3f(-edgeLength1, 0.0f, -edgeLength2);
        glTexCoord2f(uRep, 0);
        glVertex3f(-edgeLength1, 0.0f, edgeLength2);
        glTexCoord2f(uRep, vRep);
        glVertex3f(edgeLength1, 0.0f, edgeLength2);
        glTexCoord2f(0, vRep);
        glVertex3f(edgeLength1, 0.0f, -edgeLength2);
        glEnd();
    }

    void Draw(double /*t*/) override
    {
        glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
        bindTextureReflective(textureID, reflectStrength);
        glDisable(GL_LIGHTING);
        glColor4d(1, 1, 1, 1);
        drawQuadUnlit();
        if (reflectStrength > 0.02) {
            glEnable(GL_LIGHTING);
            applyFigureMaterial(1.0, reflectStrength);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawQuadUnlit();
            glDepthMask(GL_TRUE);
        }
        glPopAttrib();
    }
};

class SkySphere : public based {
    GLuint textureID;
    double sphereRadius = 1000;

public:
    SkySphere(GLuint texID, unsigned int radius = 1000) : textureID(texID), sphereRadius(radius) {}

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        out.push_back({vec<>(0, 0, 0), sphereRadius});
    }

    void Draw(double /*t*/) override
    {
        glColor4d(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        GLUquadric* quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluSphere(quad, sphereRadius, rs::sky_slc, rs::sky_stk);
        gluDeleteQuadric(quad);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }
};
