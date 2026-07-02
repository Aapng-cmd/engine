/**
 * Падения на тонкую плиту: базовые фигуры, несколько высот.
 */
#include "object_factory.h"
#include "scene.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int gFail = 0;

struct DropCase {
    std::string name;
    const char* type;
    std::vector<double> extra;
    double sx, sy, sz;
    double rx, ry, rz;
    double dropY;
    double minRestY;
    double maxRestY;
};

static Scene::ObjectPhysics gravPhys()
{
    Scene::ObjectPhysics p;
    p.gravityMode = 1;
    p.useFriction = 1;
    p.gravity = vec<>(0, -9.81, 0);
    p.restitution = 0.12;
    p.collide = 1;
    p.alpha = 1.0;
    return p;
}

static Scene::ObjectPhysics staticPhys()
{
    Scene::ObjectPhysics p = gravPhys();
    p.gravityMode = 0;
    return p;
}

static bool runDrop(const DropCase& tc)
{
    Scene scene;
    based* plate = createSceneObject("solid_cube", 0, 2, 0, 30, 1, 30, 0, 0, 0, {2.0}, 0, nullptr);
    based* body =
        createSceneObject(tc.type, 0, tc.dropY, 0, tc.sx, tc.sy, tc.sz, tc.rx, tc.ry, tc.rz, tc.extra, 0, nullptr);
    if (!plate || !body) {
        std::fprintf(stderr, "FAIL [%s]: factory\n", tc.name.c_str());
        ++gFail;
        return false;
    }
    scene.addLoadedObject(plate, staticPhys());
    scene.addLoadedObject(body, gravPhys());
    scene.physicsCameraPos = vec<>(0, 40, 60);

    const int steps = 360;
    const int settleStart = steps - 90;
    double maxVySettle = 0.0;
    for (int s = 0; s < steps; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        const Scene::BodyState& ball = scene.bodies[1];
        if (s >= settleStart)
            maxVySettle = std::max(maxVySettle, std::abs(ball.velocity.y));
        if (ball.center.y < 0.5) {
            std::fprintf(stderr, "FAIL [%s]: провал y=%.3f\n", tc.name.c_str(), ball.center.y);
            ++gFail;
            return false;
        }
    }
    const Scene::BodyState& ball = scene.bodies[1];
    if (ball.center.y < tc.minRestY || ball.center.y > tc.maxRestY) {
        std::fprintf(stderr, "FAIL [%s]: y=%.3f (ожид. %.1f…%.1f) vy=%.3f\n", tc.name.c_str(), ball.center.y,
                     tc.minRestY, tc.maxRestY, ball.velocity.y);
        ++gFail;
        return false;
    }
    if (maxVySettle > 0.35 || std::abs(ball.velocity.y) > 0.25 || ball.velocity.len() > 0.5) {
        std::fprintf(stderr, "FAIL [%s]: дрожание max|vy|=%.3f |v|=%.3f\n", tc.name.c_str(), maxVySettle,
                     ball.velocity.len());
        ++gFail;
        return false;
    }
    std::printf("OK  [%s] y=%.3f vy=%.4f\n", tc.name.c_str(), ball.center.y, ball.velocity.y);
    return true;
}

int main()
{
    const double plateTop = 2.5;
    const std::vector<double> heights = {6.0, 12.0, 22.0, 40.0};

    std::vector<DropCase> cases;
    for (double h : heights) {
        auto add = [&](const char* kind, const char* type, std::vector<double> ex, double sx, double sy, double sz,
                       double rx, double ry, double rz, double rMin, double rMax) {
            std::string name = std::string(kind) + "@" + std::to_string(static_cast<int>(h));
            cases.push_back({name, type, std::move(ex), sx, sy, sz, rx, ry, rz, h, rMin, rMax});
        };
        add("sphere", "sphere", {1.0}, 1, 1, 1, 0, 0, 0, plateTop + 0.8, plateTop + 3.0);
        add("sphere_stretched", "sphere", {1.0}, 2.8, 0.6, 1.2, 0, 25, 0, plateTop + 0.6, plateTop + 4.0);
        add("cube", "cube", {1.0, 1.0, 1.0}, 1, 1, 1, 0, 0, 0, plateTop + 0.5, plateTop + 3.5);
        add("solid_cube", "solid_cube", {1.0}, 1, 1, 1, 0, 0, 0, plateTop + 0.5, plateTop + 3.5);
        add("cylinder_up", "cylinder", {0.5, 1.0}, 1, 1, 1, 0, 0, 0, plateTop + 0.4, plateTop + 4.0);
        add("cylinder_side", "cylinder", {0.5, 1.0}, 1, 1, 1, 0, 0, 90, plateTop + 0.4, plateTop + 4.5);
        add("cone", "cone", {0.55, 1.1}, 1, 1, 1, 0, 0, 0, plateTop + 0.3, plateTop + 4.0);
        add("pyramid", "pyramid", {0.9, 1.2}, 1, 1, 1, 0, 0, 0, plateTop + 0.3, plateTop + 4.0);
        add("torus", "torus", {0.35, 1.2}, 1, 1, 1, 0, 0, 0, plateTop + 0.2, plateTop + 5.0);
    }

    cases.push_back({"long_cube@30", "cube", {1, 1, 1}, 0.9, 10, 1, 0, 0, 0, 30, plateTop + 4.0,
                     plateTop + 14.0});

    std::printf("collision suite: %zu cases\n", cases.size());
    for (const DropCase& tc : cases)
        runDrop(tc);

    if (gFail)
        std::fprintf(stderr, "\n%d test(s) failed\n", gFail);
    else
        std::printf("\nAll %zu tests passed.\n", cases.size());
    return gFail ? 1 : 0;
}
