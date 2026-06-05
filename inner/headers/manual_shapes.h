#pragma once

#include <GL/glut.h>
#include <GL/glu.h>
#include "figures.h"

/** Simple editor-driven primitives (position, euler degrees, scale, optional texture). */
struct EditorSphere : public based {
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;
    double radius = 1;
    vec<> color;

    EditorSphere(vec<> pos = {}, vec<> scale = vec<>(1, 1, 1),
                 double rx = 0, double ry = 0, double rz = 0,
                 double radius = 1, vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0);

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void Draw(double t) override;
    void drawLocal(double t);

private:
    static GLUquadric* quad;
};

struct EditorBox : public based {
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;
    double dx = 1, dy = 1, dz = 1;
    vec<> color;

    EditorBox(vec<> pos = {}, vec<> scale = vec<>(1, 1, 1),
              double rx = 0, double ry = 0, double rz = 0,
              double dx = 1, double dy = 1, double dz = 1,
              vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0);

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void Draw(double t) override;
    void drawLocal(double t);
};

struct EditorCylinder : public based {
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;
    double baseRadius = 0.5;
    double height = 1;
    vec<> color;

    EditorCylinder(vec<> pos = {}, vec<> scale = vec<>(1, 1, 1),
                   double rx = 0, double ry = 0, double rz = 0,
                   double baseRadius = 0.5, double height = 1,
                   vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0);

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void Draw(double t) override;
    void drawLocal(double t);

private:
    static GLUquadric* quad;
};

struct EditorTorus : public based {
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;
    double innerR = 0.3;
    double outerR = 1.0;
    vec<> color;

    EditorTorus(vec<> pos = {}, vec<> scale = vec<>(1, 1, 1),
                  double rx = 0, double ry = 0, double rz = 0,
                  double innerR = 0.3, double outerR = 1.0,
                  vec<> color = vec<>(0.75, 0.75, 0.75), GLuint tex = 0);

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void Draw(double t) override;
    void drawLocal(double t);
};
