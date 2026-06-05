/**
 * Длительная симуляция 2–5 тел (60 с): стабильность, без NaN/взрывов.
 */
#include "object_factory.h"
#include "scene.h"
#include "scene_loader.h"

#include <cmath>
#include <cstdio>

static int gFail = 0;

static void check(const char* name, bool ok)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL [%s]\n", name);
        ++gFail;
    } else
        std::printf("OK  [%s]\n", name);
}

static bool runSceneFile(const char* path, int steps, double maxSpeed, double maxY)
{
    Scene scene;
    if (!loadEditorSceneFile(scene, path))
        return false;
    scene.rebuildBodies();
    for (int s = 0; s < steps; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        for (const Scene::BodyState& b : scene.bodies) {
            if (!std::isfinite(b.center.x) || !std::isfinite(b.center.y) || !std::isfinite(b.center.z))
                return false;
            if (b.velocity.len() > maxSpeed)
                return false;
            if (b.center.y > maxY)
                return false;
        }
    }
    return true;
}

static bool runMultiDrop(int count, int steps)
{
    Scene scene;
    Scene::ObjectPhysics plate;
    plate.useGravity = 0;
    scene.addLoadedObject(
        createSceneObject("solid_cube", 0, 0.05, 0, 100, 0.1, 100, 0, 0, 0, {1.0}, 0, nullptr), plate);
    Scene::ObjectPhysics grav;
    grav.useGravity = 1;
    grav.gravity = vec<>(0, -9.81, 0);
    grav.collide = 1;
    const char* types[] = {"sphere", "cube", "torus", "cylinder", "cone"};
    for (int i = 0; i < count; ++i) {
        const double x = (i - count * 0.5) * 3.0;
        const char* t = types[i % 5];
        based* o = nullptr;
        if (std::string(t) == "sphere")
            o = createSceneObject(t, x, 12 + i, 0, 1, 1, 1, 0, 0, 0, {0.8}, 0, nullptr);
        else if (std::string(t) == "cube")
            o = createSceneObject(t, x, 14 + i, 0, 1, 1, 1, 0, 0, 0, {1, 1, 1}, 0, nullptr);
        else if (std::string(t) == "torus")
            o = createSceneObject(t, x, 16 + i, 0, 1, 1, 1, 0, 0, 0, {0.35, 1.0}, 0, nullptr);
        else if (std::string(t) == "cylinder")
            o = createSceneObject(t, x, 13 + i, 0, 1, 1, 1, 0, 0, 0, {0.5, 1.0}, 0, nullptr);
        else
            o = createSceneObject(t, x, 15 + i, 0, 1, 1, 1, 0, 0, 0, {0.5, 1.0}, 0, nullptr);
        scene.addLoadedObject(o, grav);
    }
    scene.rebuildBodies();
    double maxVyEnd = 0.0;
    for (int s = 0; s < steps; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        for (const Scene::BodyState& b : scene.bodies) {
            if (!std::isfinite(b.center.y) || b.center.y < -5.0 || b.center.y > 80.0)
                return false;
            if (b.velocity.len() > 40.0)
                return false;
        }
    }
    for (size_t i = 1; i < scene.bodies.size(); ++i)
        maxVyEnd = std::max(maxVyEnd, std::abs(scene.bodies[i].velocity.y));
    return maxVyEnd < 1.5;
}

int main()
{
    const int steps = 3600;
    std::printf("stress suite (%d steps = 60s)\n", steps);
    check("stress_test.scene", runSceneFile("../../stress_test.scene", steps, 25.0, 80.0));
    check("collision_test.scene", runSceneFile("../../default_collision_test.scene", steps, 25.0, 80.0));
    check("multi_2", runMultiDrop(2, steps));
    check("multi_3", runMultiDrop(3, steps));
    check("multi_5", runMultiDrop(5, steps));
    if (gFail)
        std::fprintf(stderr, "\n%d stress test(s) failed\n", gFail);
    else
        std::printf("\nAll stress tests passed.\n");
    return gFail ? 1 : 0;
}
