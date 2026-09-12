/**
 * Unity-like extras: editable mesh, scripts, factory types.
 */
#include "collision_mesh.h"
#include "editable_mesh.h"
#include "fourd_figure.h"
#include "fourd_math.h"
#include "object_factory.h"
#include "object_script_host.h"
#include "scene.h"
#include "textures_path.h"

#include <cmath>
#include <cstdio>
#include <string>

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
    std::printf("unity suite\n");

    std::string err;
    based* meshObj = createSceneObject("mesh", 0, 0, 0, 1, 1, 1, 0, 0, 0, {}, 0, &err);
    check("mesh_factory", meshObj != nullptr);
    auto* mesh = dynamic_cast<EditableMesh*>(meshObj);
    check("mesh_cast", mesh != nullptr);
    const int tris0 = mesh ? mesh->triCount() : 0;
    check("mesh_cube_tris", tris0 == 12);
    check("mesh_extrude", mesh && mesh->extrudeFace(0, 0.35) && mesh->triCount() > tris0);
    check("mesh_cap", mesh && mesh->vertCount() <= kEditableMaxVerts);

    std::vector<CollTri> ctris;
    check("mesh_collision", mesh && collision::buildObjectCollisionMesh(mesh, ctris, 4) && ctris.size() >= 12u);

    based* c16 = createSceneObject("16cell", 0, 0, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, &err);
    check("factory_16cell", c16 != nullptr);
    auto* f4 = dynamic_cast<FourDWireFigure*>(c16);
    check("16cell_tets", f4 && f4->tets.size() == 16);

    based* tes = createSceneObject("tesseract", 0, 0, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, &err);
    auto* tess = dynamic_cast<FourDWireFigure*>(tes);
    std::vector<Tet4> packed;
    copyFourDTets(tes, packed);
    check("copy_tets", packed.size() >= 40 && packed.size() <= 50);
    if (!packed.empty())
        packed[0].a.x += 0.15;
    applyFourDTets(tes, packed);
    std::vector<Tet4> packed2;
    copyFourDTets(tes, packed2);
    check("apply_tets", !packed2.empty() && std::abs(packed2[0].a.x - packed[0].a.x) < 1e-9);
    std::vector<double> blob;
    packFourDTets(packed2, blob);
    std::vector<Tet4> unpacked;
    check("tets_pack_roundtrip", unpackFourDTets(blob, unpacked) && unpacked.size() == packed2.size());

    if (tess) {
        std::vector<Vec4> uniq;
        tess->uniqueLocalVerts(uniq);
        check("unique_verts", !uniq.empty());
        const double sliceK = 0.25;
        const vec<> xyz(0.3, -0.1, 0.2);
        check("move_slice", tess->moveUniqueVertOnSlice(0, xyz, sliceK, tess->pos, 0.0));
        std::vector<Vec4> uniq2;
        tess->uniqueLocalVerts(uniq2);
        const Vec4 w2 = tess->worldVert(uniq2[0], tess->pos, 0.0);
        check("slice_k_locked", std::abs(w2.k - sliceK) < 1e-6);
        check("slice_xyz", std::abs(w2.x - xyz.x) < 1e-5 && std::abs(w2.y - xyz.y) < 1e-5 &&
                               std::abs(w2.z - xyz.z) < 1e-5);
    }

    based* cube = createSceneObject("cube", 0, 0, 0, 1, 1, 1, 0, 0, 0, {1.0, 1.0, 1.0}, 0, &err);
    std::vector<CollTri> cubeTris;
    const int cubeN = (cube && collision::buildObjectCollisionMesh(cube, cubeTris, 1))
                          ? static_cast<int>(cubeTris.size())
                          : 0;
    EditableMesh converted(vec<>(0, 0, 0), vec<>(1, 1, 1), 0, 0, 0);
    check("convert_cube_mesh", cubeN >= 12 && converted.fillFromWorldTris(cubeTris) &&
                                   converted.triCount() >= 12 && converted.vertCount() <= kEditableMaxVerts);

    Scene scene;
    Scene::ObjectPhysics p;
    p.gravityMode = 0;
    p.collide = 0;
    p.isStatic = 0;
    p.scriptPath = "scripts/orbit.so";
    based* sph = createSceneObject("sphere", 0, 3, 0, 1, 1, 1, 0, 0, 0, {1.0}, 0, &err);
    scene.addLoadedObject(sph, p);
    scene.rebuildBodies();
    const std::string so = ObjectScriptHost::resolveScriptPath("scripts/orbit.so");
    FILE* fp = std::fopen(so.c_str(), "rb");
    if (fp) {
        std::fclose(fp);
        const vec<> before = scene.bodies[0].center;
        for (int i = 0; i < 8; ++i)
            scene.stepPhysics(1.0 / 30.0);
        const vec<> after = scene.bodies[0].center;
        check("script_moves_body", (after - before).len2() > 1e-6);
    } else {
        std::printf("SKIP [script_moves_body] (no %s — run make -C inner/scripts)\n", so.c_str());
    }

    delete meshObj;
    delete c16;
    delete tes;
    delete cube;

    if (gFail)
        std::fprintf(stderr, "\n%d unity test(s) failed\n", gFail);
    else
        std::printf("\nAll unity tests passed.\n");
    return gFail ? 1 : 0;
}
