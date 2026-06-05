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
    void getBoundingSphere(vec<>& center, double& radius, double t) override;
    void emergency_bounding_sphere_calc_protocol(vec<>& center, double& radius, double t) override;
    void collectBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    based* getChild() const { return child.get(); }
    vec<> getPos() const { return pos; }
    vec<> getScale() const { return scale; }
    double getRx() const { return rx; }
    double getRy() const { return ry; }
    double getRz() const { return rz; }
    void collectCollisionSpheres(std::vector<std::pair<vec<>, double>>& out, double t) const;
};
