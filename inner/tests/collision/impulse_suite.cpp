/**
 * Импульс и движение после столкновения: линейная и угловая скорость.
 */
#include "object_factory.h"
#include "scene.h"

#include <cmath>
#include <cstdio>

static int gFail = 0;

static Scene::ObjectPhysics dynNoGrav(double restitution = 0.0)
{
    Scene::ObjectPhysics p;
    p.gravityMode = 1;
    p.gravity = vec<>(0, 0, 0);
    p.collide = 1;
    p.restitution = restitution;
    p.alpha = 1.0;
    return p;
}

static bool testHeadOnSwap()
{
    Scene scene;
    Scene::ObjectPhysics p = dynNoGrav(0.9);
    based* a = createSceneObject("sphere", -3, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    based* b = createSceneObject("sphere", 3, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    if (!a || !b) {
        std::fprintf(stderr, "FAIL [head_on_swap]: factory\n");
        ++gFail;
        return false;
    }
    scene.addLoadedObject(a, p);
    scene.addLoadedObject(b, p);
    scene.rebuildBodies();
    scene.bodies[0].velocity = vec<>(4.0, 0, 0);
    scene.bodies[1].velocity = vec<>(-4.0, 0, 0);

    for (int s = 0; s < 180; ++s)
        scene.stepPhysics(1.0 / 60.0);

    const bool aLeft = scene.bodies[0].center.x <= scene.bodies[1].center.x;
    const Scene::BodyState& left = aLeft ? scene.bodies[0] : scene.bodies[1];
    const Scene::BodyState& right = aLeft ? scene.bodies[1] : scene.bodies[0];
    if (left.velocity.x > -0.8 || right.velocity.x < 0.8) {
        std::fprintf(stderr, "FAIL [head_on_swap]: vx left=%.3f right=%.3f\n", left.velocity.x, right.velocity.x);
        ++gFail;
        return false;
    }
    std::printf("OK  [head_on_swap] vx left=%.3f right=%.3f\n", left.velocity.x, right.velocity.x);
    return true;
}

static bool testGlancingSpin()
{
    Scene scene;
    Scene::ObjectPhysics p = dynNoGrav(0.2);
    based* a = createSceneObject("sphere", -2.5, 5.2, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    based* b = createSceneObject("sphere", 2.5, 4.8, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    if (!a || !b) {
        std::fprintf(stderr, "FAIL [glancing_spin]: factory\n");
        ++gFail;
        return false;
    }
    scene.addLoadedObject(a, p);
    scene.addLoadedObject(b, p);
    scene.rebuildBodies();
    scene.bodies[0].velocity = vec<>(3.5, 0, 0);
    scene.bodies[1].velocity = vec<>(-1.0, 0, 0);

    double maxSpin = 0.0;
    for (int s = 0; s < 180; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        maxSpin = std::max(maxSpin, scene.bodies[0].angularVelocity.len());
        maxSpin = std::max(maxSpin, scene.bodies[1].angularVelocity.len());
    }
    if (maxSpin < 0.15) {
        std::fprintf(stderr, "FAIL [glancing_spin]: max|omega|=%.3f\n", maxSpin);
        ++gFail;
        return false;
    }
    std::printf("OK  [glancing_spin] max|omega|=%.3f\n", maxSpin);
    return true;
}

static bool testFloorBounce()
{
    Scene scene;
    Scene::ObjectPhysics plate = dynNoGrav(0.0);
    plate.isStatic = 1;
    Scene::ObjectPhysics ball = dynNoGrav(0.45);
    ball.gravity = vec<>(0, -9.81, 0);
    based* floor = createSceneObject("solid_cube", 0, 2, 0, 30, 1, 30, 0, 0, 0, {2.0}, 0, nullptr);
    based* body = createSceneObject("sphere", 0, 8, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    if (!floor || !body) {
        std::fprintf(stderr, "FAIL [floor_bounce]: factory\n");
        ++gFail;
        return false;
    }
    scene.addLoadedObject(floor, plate);
    scene.addLoadedObject(body, ball);
    scene.rebuildBodies();

    bool wasFalling = false;
    double minVy = 0.0;
    double peakYAfterHit = 0.0;
    for (int s = 0; s < 360; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        const double vy = scene.bodies[1].velocity.y;
        const double y = scene.bodies[1].center.y;
        if (vy < -0.5)
            wasFalling = true;
        if (wasFalling)
            peakYAfterHit = std::max(peakYAfterHit, y);
        minVy = std::min(minVy, vy);
    }
    const double finalVy = scene.bodies[1].velocity.y;
    if (!wasFalling || peakYAfterHit < 5.0) {
        std::fprintf(stderr, "FAIL [floor_bounce]: fell=%d peakY=%.3f minVy=%.3f finalVy=%.3f y=%.3f\n",
                     wasFalling ? 1 : 0, peakYAfterHit, minVy, finalVy, scene.bodies[1].center.y);
        ++gFail;
        return false;
    }
    std::printf("OK  [floor_bounce] peakY=%.3f minVy=%.3f finalVy=%.3f y=%.3f\n", peakYAfterHit, minVy, finalVy,
                scene.bodies[1].center.y);
    return true;
}

static bool testGravityOffDrift()
{
    Scene scene;
    Scene::ObjectPhysics off = dynNoGrav(0.5);
    off.gravityMode = 0;
    based* a = createSceneObject("sphere", -2, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    based* b = createSceneObject("sphere", 2, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr);
    scene.addLoadedObject(a, off);
    scene.addLoadedObject(b, off);
    scene.rebuildBodies();
    const vec<> c0 = scene.bodies[0].center;
    for (int s = 0; s < 120; ++s)
        scene.stepPhysics(1.0 / 60.0);
    const vec<> d = scene.bodies[0].center - c0;
    if (d.len() > 0.05) {
        std::fprintf(stderr, "FAIL [gravity_off_hover]: drift=%.3f\n", d.len());
        ++gFail;
        return false;
    }
    scene.bodies[0].velocity = vec<>(3.0, 0, 0);
    scene.bodies[1].velocity = vec<>(-3.0, 0, 0);
    for (int s = 0; s < 180; ++s)
        scene.stepPhysics(1.0 / 60.0);
    const double sep = (scene.bodies[1].center - scene.bodies[0].center).len();
    const double relV = (scene.bodies[1].velocity - scene.bodies[0].velocity).len();
    if (sep < 2.0 || relV < 0.5) {
        std::fprintf(stderr, "FAIL [gravity_off_collision]: sep=%.3f relV=%.3f\n", sep, relV);
        ++gFail;
        return false;
    }
    std::printf("OK  [gravity_off_hover] drift=%.4f\n", d.len());
    std::printf("OK  [gravity_off_collision] |v|=%.3f %.3f\n", scene.bodies[0].velocity.len(),
                scene.bodies[1].velocity.len());
    return true;
}

int main()
{
    std::printf("impulse suite\n");
    testHeadOnSwap();
    testGlancingSpin();
    testFloorBounce();
    testGravityOffDrift();
    if (gFail)
        std::fprintf(stderr, "\n%d impulse test(s) failed\n", gFail);
    else
        std::printf("\nAll impulse tests passed.\n");
    return gFail ? 1 : 0;
}
