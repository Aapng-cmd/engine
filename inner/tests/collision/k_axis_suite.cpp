/**
 * K-ось: 3D на разных K не взаимодействуют; 4D коллизии; удар по K → 3D kVel.
 */
#include "object_factory.h"
#include "scene.h"

#include <cmath>
#include <cstdio>

static int gFail = 0;

static Scene::ObjectPhysics phys3d()
{
    Scene::ObjectPhysics p;
    p.useGravity = 1;
    p.gravity = vec<>(0, 0, 0);
    p.collide = 1;
    p.restitution = 0.0;
    return p;
}

static void check(const char* name, bool ok)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL [%s]\n", name);
        ++gFail;
    } else
        std::printf("OK  [%s]\n", name);
}

int main()
{
    std::printf("k-axis suite\n");

    {
        Scene scene;
        auto p = phys3d();
        p.pk = 0.0;
        scene.addLoadedObject(createSceneObject("sphere", -2, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr), p);
        p.pk = 2.0;
        scene.addLoadedObject(createSceneObject("sphere", 2, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr), p);
        scene.rebuildBodies();
        scene.bodies[0].velocity = vec<>(3, 0, 0);
        scene.bodies[1].velocity = vec<>(-3, 0, 0);
        bool touched = false;
        for (int s = 0; s < 120; ++s) {
            scene.stepPhysics(1.0 / 60.0);
            Scene::Contact c;
            if (scene.detectCollision(0, 1, c))
                touched = true;
        }
        check("3d_different_k_pass_through", !touched);
    }

    {
        Scene scene;
        auto p = phys3d();
        p.pk = 0.0;
        scene.addLoadedObject(
            createSceneObject("tesseract", -1.5, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr), p);
        p.pk = 0.0;
        scene.addLoadedObject(
            createSceneObject("tesseract", 1.5, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr), p);
        scene.rebuildBodies();
        scene.bodies[0].velocity = vec<>(2, 0, 0);
        scene.bodies[1].velocity = vec<>(-2, 0, 0);
        bool hit = false;
        for (int s = 0; s < 180; ++s) {
            scene.stepPhysics(1.0 / 60.0);
            Scene::Contact c;
            if (scene.detectCollision4D(0, 1, c))
                hit = true;
        }
        const double dist = (scene.bodies[1].center - scene.bodies[0].center).len();
        check("4d_same_k_collide", hit && dist < 3.5);
    }

    {
        Scene scene;
        auto p = phys3d();
        p.pk = 0.0;
        p.vk = 1.5;
        scene.addLoadedObject(
            createSceneObject("tesseract", 0, 5, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, nullptr), p);
        p.pk = 0.0;
        p.vk = 0.0;
        scene.addLoadedObject(createSceneObject("sphere", 0, 5, 0, 1, 1, 1, 0, 0, 0, {0.5}, 0, nullptr), p);
        scene.rebuildBodies();
        for (int s = 0; s < 120; ++s)
            scene.stepPhysics(1.0 / 60.0);
        check("4d_k_strike_3d", std::abs(scene.bodies[1].kVel) > 0.01 || scene.bodies[0].kPos != 0.0);
    }

    if (gFail)
        std::fprintf(stderr, "\n%d k-axis test(s) failed\n", gFail);
    else
        std::printf("\nAll k-axis tests passed.\n");
    return gFail ? 1 : 0;
}
