#pragma once

#include "vector.h"
#include <string>
#include <vector>

/** 4D-точка: оси X, Y, Z, K (четвёртая). */
struct Vec4 {
    double x = 0, y = 0, z = 0, k = 0;
    Vec4() = default;
    Vec4(double x_, double y_, double z_, double k_) : x(x_), y(y_), z(z_), k(k_) {}
    Vec4 operator+(const Vec4& o) const { return {x + o.x, y + o.y, z + o.z, k + o.k}; }
    Vec4 operator-(const Vec4& o) const { return {x - o.x, y - o.y, z - o.z, k - o.k}; }
    Vec4 operator*(double s) const { return {x * s, y * s, z * s, k * s}; }
    double dot(const Vec4& o) const { return x * o.x + y * o.y + z * o.z + k * o.k; }
    double len2() const { return dot(*this); }
    double len() const;
    Vec4 normalized() const;
};

struct Edge4D {
    Vec4 a;
    Vec4 b;
};

/** Tetrahedron in R^4 — the 3-simplex used as a 4D “face” (Hypervis / four). */
struct Tet4 {
    Vec4 a, b, c, d;
};

/** Cap for TETS blocks in .scene (CPU slice per frame). */
constexpr int kMaxFourDTets = 80;

/** 3D triangle produced by slicing a tet with k = k0. */
struct SliceTri {
    vec<> v0, v1, v2;
};

/** Упрощённая 4D-камера (проекция Jackson Hall, встроена в движок). */
struct Camera4DState {
    Vec4 location{0, 0, -6, -2};
    Vec4 focus{0, 0, 0, 0};
    Vec4 normal{0, 0, 0.3, 1.0};
    double focalDistance = 5.0;
};

namespace fourd {

void normalizeCamera(Camera4DState& cam);
/** Синхронизация 4D-камеры с позицией/направлением 3D-наблюдателя. */
void syncViewerToCamera4d(Camera4DState& cam, const vec<>& eye, const vec<>& forward);
/** Локальная 4D-точка → мировая. rx,ry,rz — XYZ; rwx,rwy,rwz — плоскости XW/YW/ZW. */
Vec4 transformLocal4D(const Vec4& local, const vec<>& pos, const vec<>& scale, double rx, double ry, double rz,
                      double kOffset, double rwx = 0, double rwy = 0, double rwz = 0);
/** Обратное к transformLocal4D — для правки вершины в 3D-срезе (K задаёт слайсер). */
Vec4 inverseTransformLocal4D(const Vec4& world, const vec<>& pos, const vec<>& scale, double rx, double ry, double rz,
                             double kOffset, double rwx = 0, double rwy = 0, double rwz = 0);
bool projectTo3D(const Camera4DState& cam, const Vec4& p, vec<>& out);
void buildTesseract(double size, std::vector<Vec4>& verts, std::vector<Edge4D>& edges);
void buildHypersphereWire(double radius, int slices, int stacks, std::vector<Vec4>& verts,
                          std::vector<Edge4D>& edges);

/** Intersect tet with hyperplane k=k0. Returns 0, 1 (triangle) or 2 (quad as two tris). */
int sliceTet(const Tet4& tet, double k0, SliceTri out[2]);
void sliceTets(const std::vector<Tet4>& tets, double k0, std::vector<SliceTri>& out);
void buildTesseractTets(double size, std::vector<Tet4>& tets);
void build5CellTets(double size, std::vector<Tet4>& tets);
void build16CellTets(double size, std::vector<Tet4>& tets);
void tetsToEdges(const std::vector<Tet4>& tets, std::vector<Vec4>& verts, std::vector<Edge4D>& edges);

bool isFourDType(const std::string& type);

} // namespace fourd
