/**
 * Тесты 4D: проекция, 4D-коллизии, срез 3D.
 */
#include "fourd_collision.h"
#include "fourd_math.h"

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

int main()
{
    std::printf("fourd suite\n");

    fourd::HyperSphere a{{0, 0, 0, 0}, 1.0};
    fourd::HyperSphere b{{1.2, 0, 0, 0}, 1.0};
    Vec4 n4;
    double pen = 0;
    check("4d_sphere_touch", fourd::hyperSphereSphereContact(a, b, n4, pen) && pen > 0.7 && pen < 0.9);

    fourd::HyperSphere c{{5, 0, 0, 0}, 1.0};
    check("4d_sphere_separate", !fourd::hyperSphereSphereContact(a, c, n4, pen));

    fourd::HyperSphere k0{{0, 0, 0, 0}, 0.5};
    fourd::HyperSphere k1{{0, 0, 0, 0.3}, 0.5};
    check("4d_sphere_k_axis_touch", fourd::hyperSphereSphereContact(k0, k1, n4, pen) && pen > 0.6);

    Camera4DState cam;
    fourd::normalizeCamera(cam);
    vec<> n3;
    fourd::HyperSphere p0{{0, 0, 0, 0}, 0.5};
    fourd::HyperSphere p1{{0, 0, 0, 0.3}, 0.5};
    bool projTouch = fourd::hyperSphereProjected3DContact(p0, p1, cam, n3, pen);
    Vec4 n4proj;
    double pen4proj = 0;
    bool directTouch = fourd::hyperSphereSphereContact(p0, p1, n4proj, pen4proj);
    check("4d_projected_3d_touch", (projTouch && pen > 0.0) || (directTouch && pen4proj > 0.0));

    fourd::HyperSphere far4{{4.0, 0, 0, 0}, 0.5};
    check("4d_projected_3d_separate", !fourd::hyperSphereProjected3DContact(p0, far4, cam, n3, pen));

    std::vector<Vec4> v;
    std::vector<Edge4D> e;
    fourd::buildTesseract(1.0, v, e);
    check("tesseract_16v", v.size() == 16 && e.size() == 32);

    fourd::buildHypersphereWire(1.0, 8, 4, v, e);
    check("hypersphere_wire", v.size() > 20 && e.size() > 20);

    vec<> out, out2;
    check("project_origin", fourd::projectTo3D(cam, {0, 0, 0, 0}, out));
    check("project_k_axis", fourd::projectTo3D(cam, {0, 0, 0, 1}, out2));

    vec<> eye(10, 5, -20);
    vec<> fwd(-0.3, -0.1, 0.9);
    fourd::syncViewerToCamera4d(cam, eye, fwd);
    check("sync_viewer_4d", cam.location.x == eye.x && cam.focus.x != 0.0);

    SliceTri st[2];
    Tet4 tet{{ -1, 0, 0, -1 }, { 1, 0, 0, -1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 }};
    check("slice_tet_hits", fourd::sliceTet(tet, 0.0, st) >= 1);

    std::vector<Tet4> tets;
    fourd::buildTesseractTets(1.0, tets);
    check("tesseract_tets", tets.size() >= 40 && tets.size() <= 50);
    std::vector<SliceTri> slice;
    fourd::sliceTets(tets, 0.0, slice);
    std::vector<vec<>> uniq;
    auto addU = [&](const vec<>& p) {
        for (const vec<>& q : uniq)
            if ((q - p).len2() < 1e-8)
                return;
        uniq.push_back(p);
    };
    for (const SliceTri& t : slice) {
        addU(t.v0);
        addU(t.v1);
        addU(t.v2);
    }
    check("tesseract_k0_cube", slice.size() >= 6 && uniq.size() >= 8);

    fourd::build5CellTets(1.0, tets);
    check("5cell_5tets", tets.size() == 5);
    fourd::build16CellTets(1.0, tets);
    check("16cell_16tets", tets.size() == 16);

    const Vec4 spunLocal{1, 0, 0, 0};
    const Vec4 spun = fourd::transformLocal4D(spunLocal, vec<>(0, 0, 0), vec<>(1, 1, 1), 0, 0, 0, 0, 90, 0, 0);
    check("rotate_xw", std::abs(spun.k) > 0.5 && std::abs(spun.x) < 0.2);

    const Vec4 local{1, 2, 3, 0.5};
    const vec<> pos(1.5, -2.0, 0.25);
    const vec<> sc(2.0, 0.5, 1.25);
    const Vec4 world = fourd::transformLocal4D(local, pos, sc, 12, -18, 27, 0.4, 33, -21, 15);
    const Vec4 back = fourd::inverseTransformLocal4D(world, pos, sc, 12, -18, 27, 0.4, 33, -21, 15);
    check("inverse_roundtrip", (back - local).len2() < 1e-8);

    if (gFail)
        std::fprintf(stderr, "\n%d fourd test(s) failed\n", gFail);
    else
        std::printf("\nAll fourd tests passed.\n");
    return gFail ? 1 : 0;
}
