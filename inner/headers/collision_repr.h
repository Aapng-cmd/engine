#pragma once

#include "figures.h"

/** Режим коллизии объекта. */
enum class CollisionRepr {
    Sphere,   /** Быстрые сферы. */
    Triangle, /** Треугольный меш (плиты, крупные тела). */
};

/** Эвристика: сфера или треугольники по размеру и форме. */
CollisionRepr collisionReprForObject(const based* obj);

inline bool usesTriangleCollision(CollisionRepr r)
{
    return r == CollisionRepr::Triangle;
}
