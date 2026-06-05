#pragma once

#include "figures.h"
#include <vector>

struct CollisionContact {
    vec<> point;
    vec<> normal;
    double penetration = 0;
};

/** One collision triangle in local object space (before body COM offset / world pose). */
struct CollTri {
    vec<> v0;
    vec<> v1;
    vec<> v2;

    vec<> normal() const;
    double area() const;
    vec<> centroid() const;
};

namespace collision {

/** Делений ребра грани куба без --O1. */
constexpr int kFaceSubdiv = 4;

/** Включается из scene_viewer --O1: LOD коллизий по дистанции. */
extern bool gLodO1Enabled;

/** Max grid subdiv for a face of given world width (size-aware cap). */
int maxSubdivForFaceSize(double faceSize);
/** Distance LOD: near = more triangles, far = fewer (clamped by face size). */
int lodFaceSubdiv(double faceSize, double distance);

void appendBoxTriangles(double hx, double hy, double hz, int faceSubdiv, std::vector<CollTri>& out);
void appendPyramidTriangles(double baseHalf, double height, std::vector<CollTri>& out);

vec<> closestPointOnTriangle(const vec<>& p, const vec<>& a, const vec<>& b, const vec<>& c);
bool sphereTriangleContact(const vec<>& center, double radius, const CollTri& tri, CollisionContact& out);
bool triangleTriangleContact(const CollTri& a, const CollTri& b, CollisionContact& out);

/** Best outward contact (deepest valid push-out), stable on flat faces. */
bool bestSphereTriangleContact(const vec<>& center, double radius, const std::vector<CollTri>& tris,
                               CollisionContact& out, const vec<>* meshCenter = nullptr);

/** World-axis AABB (center ± halfExtents) vs sphere. */
bool sphereAabbContact(const vec<>& center, double radius, const vec<>& boxCenter, const vec<>& halfExtents,
                       CollisionContact& out);

/** Thin plate / flat box: top face only (avoids side/bottom trapping). */
bool sphereThinPlateTopContact(const vec<>& center, double radius, const vec<>& boxCenter,
                               const vec<>& halfExtents, CollisionContact& out);

/** Swept sphere motion p0→p1 vs thin-plate top (reduces tunneling). */
bool sphereThinPlateTopSwept(const vec<>& p0, const vec<>& p1, double radius, const vec<>& boxCenter,
                             const vec<>& halfExtents, CollisionContact& out);

void appendSphereTriangles(double radius, int slices, int stacks, std::vector<CollTri>& out);
void appendConeTriangles(double radius, double height, int segments, std::vector<CollTri>& out);
void appendCylinderTriangles(double radius, double height, int slices, std::vector<CollTri>& out);
void appendTorusTriangles(double tubeRadius, double ringRadius, int sides, int rings, std::vector<CollTri>& out);

/** Опора тела (меш) на верхнюю грань тонкой плиты. */
bool meshBodyOnThinPlateTop(const std::vector<CollTri>& bodyTris, const vec<>& plateCenter,
                            const vec<>& plateHalfExtents, CollisionContact& out);

/** Build collision triangles in object space (position/rotation/scale applied). */
bool buildObjectCollisionMesh(based* obj, std::vector<CollTri>& out, int faceSubdiv = kFaceSubdiv);

/** World-space mesh: vertices rotated about bodyCenter from offsets relative to pivotCenter (baseCenter). */
void buildWorldCollisionMesh(based* obj, const vec<>& bodyCenter, const vec<>& pivotCenter, const vec<>& spinAxis,
                             double spinDeg, int faceSubdiv, std::vector<CollTri>& out);

} // namespace collision
