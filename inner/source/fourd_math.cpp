#include "fourd_math.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double Vec4::len() const
{
    return std::sqrt(std::max(0.0, len2()));
}

Vec4 Vec4::normalized() const
{
    const double l = len();
    if (l < 1e-12)
        return {0, 0, 1, 0};
    return *this * (1.0 / l);
}

namespace fourd {

static Vec4 unitRight(const Camera4DState& cam)
{
    Vec4 up{0, 1, 0, 0};
    Vec4 n = cam.normal.normalized();
    Vec4 r{up.y * n.z - up.z * n.y, up.z * n.x - up.x * n.z, up.x * n.y - up.y * n.x, 0};
    const double l = r.len();
    if (l < 1e-12)
        return {1, 0, 0, 0};
    return r * (1.0 / l);
}

static Vec4 unitUp(const Camera4DState& cam)
{
    Vec4 r = unitRight(cam);
    Vec4 n = cam.normal.normalized();
    Vec4 u{r.y * n.z - r.z * n.y, r.z * n.x - r.x * n.z, r.x * n.y - r.y * n.x,
           r.y * n.k - r.k * n.y};
    const double l = u.len();
    if (l < 1e-12)
        return {0, 1, 0, 0};
    return u * (1.0 / l);
}

static Vec4 unitOut(const Camera4DState& cam)
{
    Vec4 r = unitRight(cam);
    Vec4 u = unitUp(cam);
    return {r.y * u.z - r.z * u.y, r.z * u.x - r.x * u.z, r.x * u.y - r.y * u.x,
            r.y * u.k - r.k * u.y};
}

void normalizeCamera(Camera4DState& cam)
{
    cam.normal = cam.normal.normalized();
}

static void rotateEulerXYZ(double& x, double& y, double& z, double rx, double ry, double rz)
{
    const double dx = rx * M_PI / 180.0;
    const double dy = ry * M_PI / 180.0;
    const double dz = rz * M_PI / 180.0;
    if (std::abs(dz) > 1e-12) {
        const double c = std::cos(dz), s = std::sin(dz);
        const double nx = x * c - y * s;
        const double ny = x * s + y * c;
        x = nx;
        y = ny;
    }
    if (std::abs(dy) > 1e-12) {
        const double c = std::cos(dy), s = std::sin(dy);
        const double nx = x * c + z * s;
        const double nz = -x * s + z * c;
        x = nx;
        z = nz;
    }
    if (std::abs(dx) > 1e-12) {
        const double c = std::cos(dx), s = std::sin(dx);
        const double ny = y * c - z * s;
        const double nz = y * s + z * c;
        y = ny;
        z = nz;
    }
}

Vec4 transformLocal4D(const Vec4& local, const vec<>& pos, const vec<>& scale, double rx, double ry, double rz,
                      double kOffset)
{
    double x = local.x * scale.x;
    double y = local.y * scale.y;
    double z = local.z * scale.z;
    rotateEulerXYZ(x, y, z, rx, ry, rz);
    return {x + pos.x, y + pos.y, z + pos.z, local.k + kOffset};
}

void syncViewerToCamera4d(Camera4DState& cam, const vec<>& eye, const vec<>& forward)
{
    const double k = cam.location.k;
    cam.location = {eye.x, eye.y, eye.z, k};
    const vec<> f = forward.len2() > 1e-12 ? forward * (1.0 / forward.len()) : vec<>(0, 0, -1);
    const vec<> look = eye + f * 24.0;
    cam.focus = {look.x, look.y, look.z, 0};
    cam.normal = {f.x * 0.35, f.y * 0.15, f.z * 0.35, 1.0};
    normalizeCamera(cam);
}

bool projectTo3D(const Camera4DState& cam, const Vec4& p, vec<>& out)
{
    const Vec4 focusToP = p - cam.focus;
    const Vec4 n = cam.normal.normalized();
    const Vec4 r = unitRight(cam);
    const Vec4 u = unitUp(cam);
    const Vec4 o = unitOut(cam);
    auto basisProject = [&](const Vec4& hit) {
        const Vec4 rel = hit - cam.location;
        out = vec<>(rel.dot(r), rel.dot(u), rel.dot(o));
    };
    if (focusToP.len2() < 1e-16) {
        basisProject(cam.focus);
        return true;
    }
    const double denom = n.dot(focusToP);
    if (std::abs(denom) < 1e-12)
        return false;
    if (n.dot(focusToP) <= 0.0)
        return false;
    const Vec4 camToP = p - cam.location;
    const double t = n.dot(camToP) / denom;
    basisProject(cam.location + focusToP * t);
    return true;
}

void buildTesseract(double size, std::vector<Vec4>& verts, std::vector<Edge4D>& edges)
{
    verts.clear();
    edges.clear();
    const double s = std::abs(size);
    for (int mask = 0; mask < 16; ++mask) {
        verts.push_back({(mask & 1) ? s : -s, (mask & 2) ? s : -s, (mask & 4) ? s : -s, (mask & 8) ? s : -s});
    }
    for (int i = 0; i < 16; ++i) {
        for (int j = i + 1; j < 16; ++j) {
            int diff = i ^ j;
            if ((diff & (diff - 1)) == 0)
                edges.push_back({verts[static_cast<size_t>(i)], verts[static_cast<size_t>(j)]});
        }
    }
}

void buildHypersphereWire(double radius, int slices, int stacks, std::vector<Vec4>& verts,
                          std::vector<Edge4D>& edges)
{
    verts.clear();
    edges.clear();
    const double r = std::abs(radius);
    slices = std::clamp(slices, 4, 16);
    stacks = std::clamp(stacks, 3, 12);
    for (int i = 0; i <= stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const double u = 2.0 * M_PI * j / slices;
            const double v = M_PI * (static_cast<double>(i) / stacks - 0.5);
            const double cv = std::cos(v);
            verts.push_back({r * cv * std::cos(u), r * cv * std::sin(u), r * std::sin(v), 0});
        }
    }
    const int cols = slices;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const int a = i * cols + j;
            const int b = i * cols + (j + 1) % slices;
            const int c = (i + 1) * cols + j;
            edges.push_back({verts[static_cast<size_t>(a)], verts[static_cast<size_t>(b)]});
            edges.push_back({verts[static_cast<size_t>(a)], verts[static_cast<size_t>(c)]});
        }
    }
}

bool isFourDType(const std::string& type)
{
    return type == "tesseract" || type == "hypersphere" || type == "pyramid4d";
}

} // namespace fourd
