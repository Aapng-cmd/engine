#pragma once

#include "vector.h"
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
bool projectTo3D(const Camera4DState& cam, const Vec4& p, vec<>& out);
void buildTesseract(double size, std::vector<Vec4>& verts, std::vector<Edge4D>& edges);
void buildHypersphereWire(double radius, int slices, int stacks, std::vector<Vec4>& verts,
                          std::vector<Edge4D>& edges);

bool isFourDType(const std::string& type);

} // namespace fourd
