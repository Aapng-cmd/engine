#pragma once

#include "figures.h"
#include <memory>

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
};
