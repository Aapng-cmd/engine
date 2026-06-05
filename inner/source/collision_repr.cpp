#include "collision_repr.h"
#include "manual_shapes.h"
#include "render_settings.h"
#include "transform_wrapper.h"

#include <algorithm>
#include <cmath>

static double maxAbsScale(const vec<>& s)
{
    return std::max({std::abs(s.x), std::abs(s.y), std::abs(s.z)});
}

CollisionRepr collisionReprForObject(const based* obj)
{
    if (!obj)
        return CollisionRepr::Sphere;

    if (const auto* w = dynamic_cast<const TransformWrapper*>(obj))
        return w->getChild() ? collisionReprForObject(w->getChild()) : CollisionRepr::Sphere;

    if (const auto* es = dynamic_cast<const EditorSphere*>(obj)) {
        const double r = std::abs(es->radius) * maxAbsScale(es->scale);
        if (r <= 1.25)
            return CollisionRepr::Sphere;
        if (r >= 2.5 && rs::ed_sph_slc <= 16)
            return CollisionRepr::Triangle;
        return CollisionRepr::Sphere;
    }

    if (const auto* ss = dynamic_cast<const SolidSphere*>(obj)) {
        const double r = std::abs(ss->radius);
        if (r <= 1.25)
            return CollisionRepr::Sphere;
        if (r >= 2.5)
            return CollisionRepr::Triangle;
        return CollisionRepr::Sphere;
    }

    if (dynamic_cast<const EditorTorus*>(obj) || dynamic_cast<const SolidTorus*>(obj))
        return CollisionRepr::Sphere;

    if (const auto* bx = dynamic_cast<const EditorBox*>(obj)) {
        const double ex = std::abs(bx->dx * bx->scale.x);
        const double ey = std::abs(bx->dy * bx->scale.y);
        const double ez = std::abs(bx->dz * bx->scale.z);
        const double maxE = std::max({ex, ey, ez});
        const double minE = std::min({ex, ey, ez});
        if (maxE < 1.2 && ex * ey * ez < 1.5)
            return CollisionRepr::Sphere;
        if (maxE >= 1.5 || (minE > 1e-6 && maxE / minE >= 2.5))
            return CollisionRepr::Triangle;
        return CollisionRepr::Sphere;
    }

    if (dynamic_cast<const SolidCube*>(obj) || dynamic_cast<const SolidPyramid*>(obj))
        return CollisionRepr::Triangle;

    if (const auto* cy = dynamic_cast<const EditorCylinder*>(obj)) {
        const double r = std::abs(cy->baseRadius) * std::max(std::abs(cy->scale.x), std::abs(cy->scale.z));
        const double h = std::abs(cy->height * cy->scale.y);
        if (r <= 0.65 && h <= 1.8)
            return CollisionRepr::Sphere;
        return CollisionRepr::Triangle;
    }

    if (const auto* sc = dynamic_cast<const SolidCylinder*>(obj)) {
        const double r = std::abs(sc->radius);
        const double h = std::abs(sc->height);
        if (r <= 0.65 && h <= 1.8)
            return CollisionRepr::Sphere;
        return CollisionRepr::Triangle;
    }

    if (const auto* co = dynamic_cast<const SolidCone*>(obj)) {
        const double r = std::abs(co->radius);
        const double h = std::abs(co->height);
        if (r <= 0.55 && h <= 1.4)
            return CollisionRepr::Sphere;
        return CollisionRepr::Triangle;
    }

    return CollisionRepr::Sphere;
}
