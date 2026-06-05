#pragma once

#include "figures.h"
#include <memory>
#include <vector>

/** Applies world TRS around a figure defined in local space (same order as Editor* primitives). */
class TransformWrapper : public based {
    std::unique_ptr<based> child;
    vec<> pos;
    vec<> scale;
    double rx = 0, ry = 0, rz = 0;

public:
    TransformWrapper(based* owned, vec<> pos, vec<> scale, double rx, double ry, double rz);
    void Draw(double t) override;
    /** Draw child with wrapper R/S only (position supplied by physics render). */
    void drawLocal(double t);
    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    based* getChild() const { return child.get(); }
    vec<> getPos() const { return pos; }
    vec<> getScale() const { return scale; }
    double getRx() const { return rx; }
    double getRy() const { return ry; }
    double getRz() const { return rz; }
};

void setFigureRenderAlpha(based* o, double a);
