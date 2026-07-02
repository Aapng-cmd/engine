#pragma once

#include "fourd_math.h"
#include "vector.h"

namespace fourd {

struct HyperSphere {
    Vec4 center;
    double radius = 1.0;
};

/** 4D-сфера vs 4D-сфера. */
bool hyperSphereSphereContact(const HyperSphere& a, const HyperSphere& b, Vec4& outNormal4, double& penetration);

/** Срез: проекция 4D→3D и контакт сфер в 3D (4D тела «сталкиваются через 3D»). */
bool hyperSphereProjected3DContact(const HyperSphere& a, const HyperSphere& b, const Camera4DState& cam,
                                   vec<>& outNormal3, double& penetration);

} // namespace fourd
