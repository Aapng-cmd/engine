#include "fourd_collision.h"

#include <cmath>

namespace fourd {

bool hyperSphereSphereContact(const HyperSphere& a, const HyperSphere& b, Vec4& outNormal4, double& penetration)
{
    const Vec4 d = b.center - a.center;
    const double dist = d.len();
    const double rr = a.radius + b.radius;
    if (dist > rr)
        return false;
    if (dist < 1e-12) {
        outNormal4 = {0, 0, 1, 0};
        penetration = rr;
        return true;
    }
    outNormal4 = d * (1.0 / dist);
    penetration = rr - dist;
    return penetration > 1e-9;
}

bool hyperSphereProjected3DContact(const HyperSphere& a, const HyperSphere& b, const Camera4DState& cam,
                                   vec<>& outNormal3, double& penetration)
{
    vec<> ca, cb;
    if (!projectTo3D(cam, a.center, ca) || !projectTo3D(cam, b.center, cb)) {
        // Камера может не видеть одну из точек: fallback к 3D-срезу по XYZ.
        ca = vec<>(a.center.x, a.center.y, a.center.z);
        cb = vec<>(b.center.x, b.center.y, b.center.z);
    }
    vec<> d = cb - ca;
    const double dist = d.len();
    const double rr = a.radius + b.radius;
    if (dist > rr)
        return false;
    if (dist < 1e-12) {
        outNormal3 = vec<>(0, 1, 0);
        penetration = rr;
        return true;
    }
    outNormal3 = d * (1.0 / dist);
    penetration = rr - dist;
    return penetration > 1e-9;
}

} // namespace fourd
