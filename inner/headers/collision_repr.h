#pragma once

#include "figures.h"

/** How physics builds collision bounds for an object. */
enum class CollisionRepr {
    Sphere,   /** Fast analytic sphere(s). */
    Triangle, /** Subdivided triangle mesh (planes, large solids). */
};

/** Choose sphere vs triangle collision from shape size, aspect ratio, and tessellation. */
CollisionRepr collisionReprForObject(const based* obj);

inline bool usesTriangleCollision(CollisionRepr r)
{
    return r == CollisionRepr::Triangle;
}
