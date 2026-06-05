#include "collision_mesh.h"
#include "collision_repr.h"
#include "object_factory.h"
#include "scene.h"

#include <cstdio>
#include <cmath>
#include <vector>

static int runSteps = 180;

int main()
{
    Scene scene;
    Scene::ObjectPhysics phys;
    phys.useGravity = 1;
    phys.gravity = vec<>(0, -9.81, 0);

    Scene::ObjectPhysics platePhys = phys;
    platePhys.useGravity = 0;
    std::vector<double> cubeEx = {1.0};
    based* cube = createSceneObject("cube", 0, 2, 0, 10, 1, 10, 0, 0, 0, cubeEx, 0);
    std::vector<double> sphEx = {1.0};
    based* sph = createSceneObject("sphere", 0, 10, 0, 1, 1, 1, 0, 0, 0, sphEx, 0);
    if (!cube || !sph) {
        std::fprintf(stderr, "factory failed\n");
        return 1;
    }
    scene.addLoadedObject(cube, platePhys);
    scene.addLoadedObject(sph, phys);
    scene.physicsCameraPos = vec<>(0, 25, 40);
    scene.rebuildBodies();

    std::printf("collision_test: %zu bodies\n", scene.bodies.size());
    for (size_t i = 0; i < scene.bodies.size(); ++i) {
        const Scene::BodyState& b = scene.bodies[i];
        std::printf("  [%zu] repr=%s y=%.2f r=%.2f he=(%.2f,%.2f,%.2f) tris=%zu\n", i,
                    b.collisionRepr == CollisionRepr::Triangle ? "triangle" : "sphere", b.center.y, b.radius,
                    b.halfExtents.x, b.halfExtents.y, b.halfExtents.z, b.partsTriLocal.size());
    }

    for (int s = 0; s < runSteps; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        const Scene::BodyState& plate = scene.bodies[0];
        const Scene::BodyState& ball = scene.bodies[1];
        if (s % 30 == 0)
            std::printf("step %3d ball y=%.4f vy=%.4f\n", s, ball.center.y, ball.velocity.y);
        if (ball.center.y < plate.center.y - 2.0) {
            std::fprintf(stderr, "FAIL: ball fell through plate (y=%.3f)\n", ball.center.y);
            return 2;
        }
    }
    const double restY = scene.bodies[1].center.y;
    if (restY < 2.5 || restY > 5.5) {
        std::fprintf(stderr, "FAIL: rest y=%.3f expected ~3.5\n", restY);
        return 3;
    }
    std::printf("OK: ball rests on plate at y=%.3f\n", restY);
    return 0;
}
