/**
 * Слайдер детализации коллизий (collisionSubdiv) и полностью статические тела.
 *
 * Проверяем три свойства:
 *  1) слайдер реально меняет сетку у каждой фигуры (и не упирается в потолок раньше 24);
 *  2) при любой детализации тело ложится на опору на той же высоте и не дрожит;
 *  3) составная фигура (группа) держит единый subdiv на всех частях;
 *  4) isStatic-тело не двигается вообще, даже после удара.
 */
#include "collision_mesh.h"
#include "object_factory.h"
#include "scene.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int gFail = 0;

static bool check(const std::string& name, bool ok, const std::string& detail = {})
{
    if (ok) {
        std::printf("OK  [%s]%s%s\n", name.c_str(), detail.empty() ? "" : " ", detail.c_str());
        return true;
    }
    std::fprintf(stderr, "FAIL [%s] %s\n", name.c_str(), detail.c_str());
    ++gFail;
    return false;
}

static Scene::ObjectPhysics dynPhys(int subdiv)
{
    Scene::ObjectPhysics p;
    p.gravityMode = 1;
    p.useFriction = 1;
    p.gravity = vec<>(0, -9.81, 0);
    p.restitution = 0.12;
    p.collide = 1;
    p.alpha = 1.0;
    p.collisionSubdiv = subdiv;
    return p;
}

static Scene::ObjectPhysics platePhys()
{
    Scene::ObjectPhysics p = dynPhys(4);
    p.gravityMode = 0;
    p.isStatic = 1;
    return p;
}

struct ShapeCase {
    const char* name;
    const char* type;
    std::vector<double> extra;
    double sx, sy, sz, rx, ry, rz;
};

static int triCount(const ShapeCase& sc, int subdiv)
{
    based* o = createSceneObject(sc.type, 0, 0, 0, sc.sx, sc.sy, sc.sz, sc.rx, sc.ry, sc.rz, sc.extra, 0, nullptr);
    if (!o)
        return -1;
    std::vector<CollTri> tris;
    const bool ok = collision::buildObjectCollisionMesh(o, tris, subdiv);
    delete o;
    return ok ? static_cast<int>(tris.size()) : -1;
}

/** Кладём фигуру на статическую плиту, возвращаем высоту покоя и максимальную дрожь. */
static bool settle(const ShapeCase& sc, int subdiv, double& restY, double& maxVySettle)
{
    Scene scene;
    based* plate = createSceneObject("solid_cube", 0, 2, 0, 30, 1, 30, 0, 0, 0, {2.0}, 0, nullptr);
    based* body = createSceneObject(sc.type, 0, 8.0, 0, sc.sx, sc.sy, sc.sz, sc.rx, sc.ry, sc.rz, sc.extra, 0, nullptr);
    if (!plate || !body)
        return false;
    scene.addLoadedObject(plate, platePhys());
    scene.addLoadedObject(body, dynPhys(subdiv));
    scene.physicsCameraPos = vec<>(0, 40, 60);

    maxVySettle = 0.0;
    for (int s = 0; s < 360; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        if (s >= 270)
            maxVySettle = std::max(maxVySettle, std::abs(scene.bodies[1].velocity.y));
    }
    restY = scene.bodies[1].center.y;
    return true;
}

static void testSliderRange()
{
    const ShapeCase shapes[] = {
        {"sphere", "sphere", {1.0}, 1, 1, 1, 0, 0, 0},
        {"cube", "cube", {1, 1, 1}, 1, 1, 1, 0, 0, 0},
        {"solid_cube", "solid_cube", {1.0}, 1, 1, 1, 0, 0, 0},
        {"cylinder", "cylinder", {0.5, 1.0}, 1, 1, 1, 0, 0, 0},
        {"cone", "cone", {0.55, 1.1}, 1, 1, 1, 0, 0, 0},
        {"pyramid", "pyramid", {0.9, 1.2}, 1, 1, 1, 0, 0, 0},
        {"torus", "torus", {0.35, 2.2}, 1, 1, 1, 0, 0, 0},
        {"torus_scaled", "torus", {0.35, 2.2}, 1, 2, 3, 90, 0, 0},
    };
    const int subdivs[] = {1, 2, 4, 8, 12, 16, 24};

    for (const ShapeCase& sc : shapes) {
        int prev = -1;
        bool monotonic = true;
        for (int s : subdivs) {
            const int n = triCount(sc, s);
            if (n <= 0) {
                monotonic = false;
                break;
            }
            if (prev >= 0 && n < prev)
                monotonic = false;
            prev = n;
        }
        const int low = triCount(sc, 1);
        const int high = triCount(sc, 24);
        char buf[160];
        std::snprintf(buf, sizeof(buf), "s1=%d s24=%d (x%.1f)", low, high,
                      low > 0 ? static_cast<double>(high) / low : 0.0);
        /* Потолок ниже 24 означал бы мёртвый ход слайдера на верхней половине. */
        check(std::string("slider_range/") + sc.name, monotonic && low > 0 && high >= low * 2, buf);

        /* Верхняя треть слайдера тоже должна что-то менять. */
        const int mid = triCount(sc, 16);
        check(std::string("slider_top_half/") + sc.name, high > mid,
              "s16=" + std::to_string(mid) + " s24=" + std::to_string(high));
    }
}

static void testSettleAcrossSubdiv()
{
    const ShapeCase shapes[] = {
        {"sphere", "sphere", {1.0}, 1, 1, 1, 0, 0, 0},
        {"cube", "cube", {1, 1, 1}, 1, 1, 1, 0, 0, 0},
        {"cylinder", "cylinder", {0.5, 1.0}, 1, 1, 1, 0, 0, 0},
        {"cone", "cone", {0.55, 1.1}, 1, 1, 1, 0, 0, 0},
        {"pyramid", "pyramid", {0.9, 1.2}, 1, 1, 1, 0, 0, 0},
        {"torus", "torus", {0.35, 2.2}, 1, 1, 1, 0, 0, 0},
        {"torus_scaled", "torus", {0.35, 2.2}, 1, 2, 3, 90, 0, 0},
    };
    const int subdivs[] = {1, 4, 12, 24};

    for (const ShapeCase& sc : shapes) {
        double baseY = 0.0, jitter = 0.0;
        if (!settle(sc, 4, baseY, jitter)) {
            check(std::string("settle/") + sc.name, false, "factory");
            continue;
        }
        bool ok = true;
        double worstDelta = 0.0, worstJitter = 0.0;
        for (int s : subdivs) {
            double y = 0.0, vy = 0.0;
            if (!settle(sc, s, y, vy)) {
                ok = false;
                break;
            }
            worstDelta = std::max(worstDelta, std::abs(y - baseY));
            worstJitter = std::max(worstJitter, vy);
            /* Разная детализация не должна менять высоту покоя более чем на толщину грани. */
            if (std::abs(y - baseY) > 0.35 || vy > 0.35 || y < 3.0)
                ok = false;
        }
        char buf[160];
        std::snprintf(buf, sizeof(buf), "y=%.3f maxΔ=%.3f maxVy=%.3f", baseY, worstDelta, worstJitter);
        check(std::string("settle_across_subdiv/") + sc.name, ok, buf);
    }
}

/** Составная фигура: части объединены groupId и должны делить один subdiv. */
static void testCompoundGroup()
{
    auto buildScene = [](int subdiv, Scene& scene) {
        based* plate = createSceneObject("solid_cube", 0, 2, 0, 30, 1, 30, 0, 0, 0, {2.0}, 0, nullptr);
        based* partA = createSceneObject("cube", -0.6, 8.0, 0, 1, 1, 1, 0, 0, 0, {1, 1, 1}, 0, nullptr);
        based* partB = createSceneObject("sphere", 0.9, 8.0, 0, 1, 1, 1, 0, 0, 0, {0.6}, 0, nullptr);
        if (!plate || !partA || !partB)
            return false;
        Scene::ObjectPhysics a = dynPhys(subdiv);
        a.groupId = 7;
        Scene::ObjectPhysics b = dynPhys(subdiv);
        b.groupId = 7;
        scene.addLoadedObject(plate, platePhys());
        scene.addLoadedObject(partA, a);
        scene.addLoadedObject(partB, b);
        scene.physicsCameraPos = vec<>(0, 40, 60);
        return true;
    };

    size_t lowTotal = 0, highTotal = 0;
    for (int subdiv : {2, 20}) {
        Scene scene;
        if (!buildScene(subdiv, scene)) {
            check("compound/factory", false);
            return;
        }
        scene.stepPhysics(1.0 / 60.0);
        size_t total = 0;
        bool sameSubdiv = true;
        for (size_t i = 1; i < scene.bodies.size(); ++i) {
            total += scene.bodies[i].partsTriLocal.size();
            if (scene.bodies[i].collisionSubdiv != subdiv)
                sameSubdiv = false;
        }
        check("compound/uniform_subdiv@" + std::to_string(subdiv), sameSubdiv,
              "parts=" + std::to_string(scene.bodies.size() - 1) + " tris=" + std::to_string(total));
        (subdiv == 2 ? lowTotal : highTotal) = total;
    }
    /* Один слайдер должен поднимать плотность всей фигуры, а не одной её части. */
    check("compound/total_scales", highTotal > lowTotal * 2,
          "tris " + std::to_string(lowTotal) + " -> " + std::to_string(highTotal));
}

/** Полностью статическое тело: не падает, не сдвигается ударом. */
static void testStaticBody()
{
    Scene scene;
    based* plate = createSceneObject("solid_cube", 0, 2, 0, 30, 1, 30, 0, 0, 0, {2.0}, 0, nullptr);
    based* heavy = createSceneObject("sphere", 0, 9.0, 0, 1, 1, 1, 0, 0, 0, {1.2}, 0, nullptr);
    if (!plate || !heavy) {
        check("static/factory", false);
        return;
    }
    scene.addLoadedObject(plate, platePhys());
    Scene::ObjectPhysics hp = dynPhys(4);
    hp.massOverride = 500.0;
    scene.addLoadedObject(heavy, hp);
    scene.physicsCameraPos = vec<>(0, 40, 60);

    scene.stepPhysics(1.0 / 60.0);
    const vec<> start = scene.bodies[0].center;
    double maxDrift = 0.0, maxSpeed = 0.0;
    for (int s = 0; s < 600; ++s) {
        scene.stepPhysics(1.0 / 60.0);
        maxDrift = std::max(maxDrift, (scene.bodies[0].center - start).len());
        maxSpeed = std::max(maxSpeed, scene.bodies[0].velocity.len());
    }
    char buf[128];
    std::snprintf(buf, sizeof(buf), "drift=%.6f maxSpeed=%.6f", maxDrift, maxSpeed);
    check("static/immovable_under_impact", maxDrift < 1e-9 && maxSpeed < 1e-9, buf);

    /* Статическое тело не должно падать даже при включённой гравитации. */
    Scene g;
    based* floating = createSceneObject("cube", 0, 12, 0, 1, 1, 1, 0, 0, 0, {1, 1, 1}, 0, nullptr);
    if (!floating) {
        check("static/factory2", false);
        return;
    }
    Scene::ObjectPhysics sp = dynPhys(4);
    sp.isStatic = 1;
    g.addLoadedObject(floating, sp);
    g.physicsCameraPos = vec<>(0, 40, 60);
    const double y0 = 12.0;
    for (int s = 0; s < 300; ++s)
        g.stepPhysics(1.0 / 60.0);
    std::snprintf(buf, sizeof(buf), "y=%.6f", g.bodies[0].center.y);
    check("static/ignores_gravity", std::abs(g.bodies[0].center.y - y0) < 1e-9, buf);
}

/** Визуальный конус и коллизия делят одну систему: COM сетки около начала координат. */
static void testConeMeshCentered()
{
    based* o = createSceneObject("cone", 0, 0, 0, 1, 1, 1, 0, 0, 0, {0.8, 1.6}, 0, nullptr);
    if (!o) {
        check("cone_mesh/factory", false);
        return;
    }
    std::vector<CollTri> tris;
    const bool ok = collision::buildObjectCollisionMesh(o, tris, 8);
    delete o;
    if (!ok || tris.empty()) {
        check("cone_mesh/build", false);
        return;
    }
    double ymin = 1e9, ymax = -1e9;
    for (const CollTri& t : tris) {
        for (const vec<>* v : {&t.v0, &t.v1, &t.v2}) {
            ymin = std::min(ymin, v->y);
            ymax = std::max(ymax, v->y);
        }
    }
    /* Как drawConeTextured: основание y=-h/2, вершина y=+h/2 (h=1.6 → ±0.8). */
    char buf[128];
    std::snprintf(buf, sizeof(buf), "tris=%zu y=[%.3f,%.3f]", tris.size(), ymin, ymax);
    check("cone_mesh/matches_visual_extent", std::abs(ymin + 0.8) < 0.05 && std::abs(ymax - 0.8) < 0.05, buf);
}

int main()
{
    std::printf("subdiv suite\n");
    testSliderRange();
    testSettleAcrossSubdiv();
    testCompoundGroup();
    testStaticBody();
    testConeMeshCentered();

    if (gFail) {
        std::fprintf(stderr, "\n%d subdiv test(s) failed.\n", gFail);
        return 1;
    }
    std::printf("\nAll subdiv tests passed.\n");
    return 0;
}
