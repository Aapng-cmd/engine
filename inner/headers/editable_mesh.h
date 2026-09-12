#pragma once

#include "engine_power.h"
#include "figures.h"
#include "collision_mesh.h"
#include <algorithm>
#include <vector>

/** In-engine triangle mesh (mini-Blender). Caps keep the CPU cheap. */
constexpr int kEditableMaxVerts = 256;
constexpr int kEditableMaxTris = 512;

struct EditableMesh : public based {
    vec<> pos{0, 0, 0};
    vec<> scale{1, 1, 1};
    double rx = 0, ry = 0, rz = 0;
    vec<> color{0.75, 0.75, 0.75};
    std::vector<vec<>> verts;
    std::vector<int> indices; /* triples */

    EditableMesh(vec<> p, vec<> s, double rx_, double ry_, double rz_, GLuint tex = 0)
        : pos(p), scale(s), rx(rx_), ry(ry_), rz(rz_)
    {
        textureID = tex;
    }

    int vertCount() const { return static_cast<int>(verts.size()); }
    int triCount() const { return static_cast<int>(indices.size() / 3); }

    bool canAddVerts(int n) const { return vertCount() + n <= engine::maxMeshVerts(); }
    bool canAddTris(int n) const { return triCount() + n <= engine::maxMeshTris(); }

    void fillUnitCube();
    void fillPlane(double hx = 1, double hz = 1);
    /** Extrude triangle `face` along its normal by `amount`. Returns false if cap hit. */
    bool extrudeFace(int face, double amount = 0.35);
    void setVertex(int i, const vec<>& p);

    vec<> worldVertex(int i) const;
    /** Cheap planar UV from triangle normal (no unwrap). */
    static void triangleUv(const vec<>& a, const vec<>& b, const vec<>& c, double uv[3][2]);
    /** Fill from collision triangles (weld + cap). */
    bool fillFromWorldTris(const std::vector<CollTri>& tris);

    void getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t) override;
    void drawLocal(double t);
    void Draw(double t) override;
};

void applyEditableMeshData(based* o, const std::vector<vec<>>& verts, const std::vector<int>& indices);
void copyEditableMeshData(const based* o, std::vector<vec<>>& verts, std::vector<int>& indices);
