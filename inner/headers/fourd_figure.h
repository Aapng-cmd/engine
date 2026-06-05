#pragma once

#include "figures.h"
#include "fourd_math.h"
#include <string>
#include <vector>

/** 4D-каркас: рёбра проецируются 4D→3D при отрисовке. */
class FourDWireFigure : public based {
public:
    std::string shapeType;
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;
    double sizeParam = 1.0;
    double kPos = 0.0;
    vec<> color{0.75, 0.75, 0.85};
    std::vector<Vec4> verts4;
    std::vector<Edge4D> edges4;

    FourDWireFigure(const std::string& type, vec<> p, vec<> s, double rx_, double ry_, double rz_, double size,
                    vec<> col, GLuint tex = 0);

    void rebuildGeometry();
    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void Draw(double t) override;
    void drawProjected(const Camera4DState& cam, double kWorld = 0.0) const;
};
