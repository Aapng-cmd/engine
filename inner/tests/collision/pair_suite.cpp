/**
 * Пары тел без гравитации: контакт без притяжения и проникновения.
 */
#include "object_factory.h"
#include "scene.h"

#include <cmath>
#include <cstdio>
#include <string>

static int gFail = 0;

static Scene::ObjectPhysics noGrav()
{
    Scene::ObjectPhysics p;
    p.useGravity = 1;
    p.gravity = vec<>(0, 0, 0);
    p.collide = 1;
    p.restitution = 0.0;
    return p;
}

static bool runPair(const char* name, based* a, based* b, double minDist, double maxDist)
{
    Scene scene;
    scene.addLoadedObject(a, noGrav());
    scene.addLoadedObject(b, noGrav());
    scene.rebuildBodies();
    if (scene.bodies.size() != 2) {
        std::fprintf(stderr, "FAIL [%s]: bodies\n", name);
        ++gFail;
        return false;
    }
    scene.bodies[0].velocity = vec<>(2.0, 0, 0);
    scene.bodies[1].velocity = vec<>(-2.0, 0, 0);
    double maxPen = 0.0;
    double maxAttract = 0.0;
    for (int s = 0; s < 240; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        const vec<> d = scene.bodies[1].center - scene.bodies[0].center;
        const double dist = d.len();
        Scene::Contact cc;
        const double pen = scene.detectCollision(0, 1, cc) ? cc.penetration : 0.0;
        if (pen > maxPen)
            maxPen = pen;
        const double sepVel = (scene.bodies[1].velocity - scene.bodies[0].velocity).dot(d * (1.0 / std::max(1e-9, dist)));
        if (pen > 0.05 && sepVel < -0.5)
            maxAttract += 1.0;
    }
    const vec<> d = scene.bodies[1].center - scene.bodies[0].center;
    const double dist = d.len();
    if (dist < minDist || dist > maxDist) {
        std::fprintf(stderr, "FAIL [%s]: dist=%.3f (%.1f…%.1f) pen=%.3f attract=%.0f\n", name, dist, minDist,
                     maxDist, maxPen, maxAttract);
        ++gFail;
        return false;
    }
    if (maxPen > 0.5 || maxAttract > 12.0) {
        std::fprintf(stderr, "FAIL [%s]: pen=%.3f attract=%.0f\n", name, maxPen, maxAttract);
        ++gFail;
        return false;
    }
    std::printf("OK  [%s] dist=%.3f pen=%.3f\n", name, dist, maxPen);
    return true;
}

int main()
{
    std::printf("pair suite\n");
    runPair("cube+cube",
            createSceneObject("cube", -3, 5, 0, 1, 1, 1, 0, 0, 0, {1, 1, 1}, 0, nullptr),
            createSceneObject("cube", 3, 5, 0, 1, 1, 1, 0, 0, 0, {1, 1, 1}, 0, nullptr), 1.5, 4.0);
    runPair("torus+long_cube",
            createSceneObject("torus", -4, 8, 0, 1, 1, 1, 0, 0, 0, {0.35, 2.2}, 0, nullptr),
            createSceneObject("cube", 4, 8, 0, 0.9, 10, 1, 0, 0, 0, {1, 1, 1}, 0, nullptr), 2.0, 12.0);
    runPair("sphere+sphere",
            createSceneObject("sphere", -2, 3, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr),
            createSceneObject("sphere", 2, 3, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr), 1.8, 3.5);
    if (gFail)
        std::fprintf(stderr, "\n%d pair test(s) failed\n", gFail);
    else
        std::printf("\nAll pair tests passed.\n");
    return gFail ? 1 : 0;
}
