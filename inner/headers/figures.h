#pragma once

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

namespace physmath {
inline double sphereVolume(double r) { return (4.0 / 3.0) * M_PI * r * r * r; }
inline double boxVolume(double x, double y, double z) { return x * y * z; }
inline double cylinderVolume(double r, double h) { return M_PI * r * r * h; }
inline double coneVolume(double r, double h) { return (M_PI * r * r * h) / 3.0; }
} // namespace physmath

struct based {
    double renderAlpha = 1.0;
    GLuint textureID = 0;
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
        glutSolidCube(1.0);
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
        glPushMatrix();
        glRotated(-90, 1, 0, 0);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, renderAlpha);
            gluCylinder(quad, radius, 0, height, rs::cone_seg, rs::cone_seg);
            glDisable(GL_TEXTURE_2D);
        } else {
            glColor4d(color.x, color.y, color.z, renderAlpha);
            glutSolidCone(radius, height, rs::cone_seg, rs::cone_seg);
        }
        glPopMatrix();
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
        glPushMatrix();
        glTranslated(0, h * 0.5, 0);
        glBegin(GL_TRIANGLES);
        auto tri = [&](double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3,
                       double z3) {
            glVertex3d(x1, y1, z1);
            glVertex3d(x2, y2, z2);
            glVertex3d(x3, y3, z3);
        };
        tri(-a, -h * 0.5, -a, a, -h * 0.5, -a, 0, h * 0.5, 0);
        tri(a, -h * 0.5, -a, a, -h * 0.5, a, 0, h * 0.5, 0);
        tri(a, -h * 0.5, a, -a, -h * 0.5, a, 0, h * 0.5, 0);
        tri(-a, -h * 0.5, a, -a, -h * 0.5, -a, 0, h * 0.5, 0);
        glEnd();
        glPopMatrix();
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
        glutSolidTorus(innerR, outerR, rs::ed_tor_s, rs::ed_tor_r);
        if (textureID != 0)
            glDisable(GL_TEXTURE_2D);
    }
};

class GroundPlane : public based {
    GLuint textureID;
    int edgeLength1 = 200;
    int edgeLength2 = 200;

public:
    GroundPlane(GLuint texID, int e1 = 200, int e2 = 200) : textureID(texID), edgeLength1(e1), edgeLength2(e2) {}

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/) override
    {
        out.push_back({vec<>(0, 0, 0),
                       std::sqrt(static_cast<double>(edgeLength1 * edgeLength1 + edgeLength2 * edgeLength2))});
    }

    void Draw(double /*t*/) override
    {
        glColor4d(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(-edgeLength1, -0.001f, -edgeLength2);
        glTexCoord2f(1, 0);
        glVertex3f(-edgeLength1, -0.001f, edgeLength2);
        glTexCoord2f(1, 1);
        glVertex3f(edgeLength1, -0.001f, edgeLength2);
        glTexCoord2f(0, 1);
        glVertex3f(edgeLength1, -0.001f, -edgeLength2);
        glEnd();
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
