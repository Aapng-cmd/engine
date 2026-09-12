#include "fourd_math.h"

#include <algorithm>
#include <cmath>
#include <string>

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
                      double kOffset, double rwx, double rwy, double rwz)
{
    double x = local.x * scale.x;
    double y = local.y * scale.y;
    double z = local.z * scale.z;
    double k = local.k;
    rotateEulerXYZ(x, y, z, rx, ry, rz);
    const double dwx = rwx * M_PI / 180.0;
    if (std::abs(dwx) > 1e-12) {
        const double c = std::cos(dwx), s = std::sin(dwx);
        const double nx = x * c - k * s;
        const double nk = x * s + k * c;
        x = nx;
        k = nk;
    }
    const double dwy = rwy * M_PI / 180.0;
    if (std::abs(dwy) > 1e-12) {
        const double c = std::cos(dwy), s = std::sin(dwy);
        const double ny = y * c - k * s;
        const double nk = y * s + k * c;
        y = ny;
        k = nk;
    }
    const double dwz = rwz * M_PI / 180.0;
    if (std::abs(dwz) > 1e-12) {
        const double c = std::cos(dwz), s = std::sin(dwz);
        const double nz = z * c - k * s;
        const double nk = z * s + k * c;
        z = nz;
        k = nk;
    }
    return {x + pos.x, y + pos.y, z + pos.z, k + kOffset};
}

static void unrotateEulerXYZ(double& x, double& y, double& z, double rx, double ry, double rz)
{
    /* Forward rotateEulerXYZ applies Z then Y then X. Inverse: -X, -Y, -Z. */
    const double dx = -rx * M_PI / 180.0;
    const double dy = -ry * M_PI / 180.0;
    const double dz = -rz * M_PI / 180.0;
    if (std::abs(dx) > 1e-12) {
        const double c = std::cos(dx), s = std::sin(dx);
        const double ny = y * c - z * s;
        const double nz = y * s + z * c;
        y = ny;
        z = nz;
    }
    if (std::abs(dy) > 1e-12) {
        const double c = std::cos(dy), s = std::sin(dy);
        const double nx = x * c + z * s;
        const double nz = -x * s + z * c;
        x = nx;
        z = nz;
    }
    if (std::abs(dz) > 1e-12) {
        const double c = std::cos(dz), s = std::sin(dz);
        const double nx = x * c - y * s;
        const double ny = x * s + y * c;
        x = nx;
        y = ny;
    }
}

Vec4 inverseTransformLocal4D(const Vec4& world, const vec<>& pos, const vec<>& scale, double rx, double ry, double rz,
                             double kOffset, double rwx, double rwy, double rwz)
{
    double x = world.x - pos.x;
    double y = world.y - pos.y;
    double z = world.z - pos.z;
    double k = world.k - kOffset;
    auto plane = [](double& a, double& kk, double deg) {
        const double d = -deg * M_PI / 180.0;
        if (std::abs(d) <= 1e-12)
            return;
        const double c = std::cos(d), s = std::sin(d);
        const double na = a * c - kk * s;
        const double nk = a * s + kk * c;
        a = na;
        kk = nk;
    };
    plane(z, k, rwz);
    plane(y, k, rwy);
    plane(x, k, rwx);
    unrotateEulerXYZ(x, y, z, rx, ry, rz);
    const double sx = std::abs(scale.x) > 1e-12 ? scale.x : 1.0;
    const double sy = std::abs(scale.y) > 1e-12 ? scale.y : 1.0;
    const double sz = std::abs(scale.z) > 1e-12 ? scale.z : 1.0;
    return {x / sx, y / sy, z / sz, k};
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
    slices = std::clamp(slices, 4, 32);
    stacks = std::clamp(stacks, 3, 24);
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
    return type == "tesseract" || type == "hypersphere" || type == "pyramid4d" || type == "16cell";
}

static void pushUnique(std::vector<vec<>>& pts, const vec<>& p)
{
    for (const vec<>& q : pts) {
        if ((q - p).len2() < 1e-12)
            return;
    }
    pts.push_back(p);
}

int sliceTet(const Tet4& tet, double k0, SliceTri out[2])
{
    const Vec4 v[4] = {tet.a, tet.b, tet.c, tet.d};
    const int e[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    std::vector<vec<>> hits;
    hits.reserve(6);
    auto consider = [&](const Vec4& p) { pushUnique(hits, vec<>(p.x, p.y, p.z)); };
    for (int i = 0; i < 4; ++i) {
        if (std::abs(v[i].k - k0) < 1e-9)
            consider(v[i]);
    }
    for (int ei = 0; ei < 6; ++ei) {
        const Vec4& a = v[e[ei][0]];
        const Vec4& b = v[e[ei][1]];
        const double da = a.k - k0;
        const double db = b.k - k0;
        if (da * db > 1e-18)
            continue;
        if (std::abs(da) < 1e-9 && std::abs(db) < 1e-9) {
            consider(a);
            consider(b);
            continue;
        }
        const double denom = b.k - a.k;
        if (std::abs(denom) < 1e-18)
            continue;
        const double t = std::clamp((k0 - a.k) / denom, 0.0, 1.0);
        const Vec4 p = a + (b - a) * t;
        consider(p);
    }
    if (hits.size() < 3)
        return 0;
    if (hits.size() == 3) {
        out[0] = {hits[0], hits[1], hits[2]};
        return 1;
    }
    /* 4 hits: split the quad. */
    out[0] = {hits[0], hits[1], hits[2]};
    out[1] = {hits[0], hits[2], hits[3]};
    return 2;
}

void sliceTets(const std::vector<Tet4>& tets, double k0, std::vector<SliceTri>& out)
{
    out.clear();
    SliceTri tmp[2];
    for (const Tet4& tet : tets) {
        const int n = sliceTet(tet, k0, tmp);
        for (int i = 0; i < n; ++i)
            out.push_back(tmp[i]);
    }
}

static void tetCube(const Vec4& o, const Vec4& ax, const Vec4& ay, const Vec4& az, std::vector<Tet4>& tets)
{
    /* 5-tet split of a cube with origin o and edges ax,ay,az. */
    const Vec4 p000 = o;
    const Vec4 p100 = o + ax;
    const Vec4 p010 = o + ay;
    const Vec4 p110 = o + ax + ay;
    const Vec4 p001 = o + az;
    const Vec4 p101 = o + ax + az;
    const Vec4 p011 = o + ay + az;
    const Vec4 p111 = o + ax + ay + az;
    tets.push_back({p000, p100, p010, p001});
    tets.push_back({p100, p110, p010, p111});
    tets.push_back({p100, p010, p001, p111});
    tets.push_back({p100, p101, p001, p111});
    tets.push_back({p010, p011, p001, p111});
}

void buildTesseractTets(double size, std::vector<Tet4>& tets)
{
    tets.clear();
    const double s = std::abs(size);
    const Vec4 o{-s, -s, -s, -s};
    const Vec4 ax{2 * s, 0, 0, 0};
    const Vec4 ay{0, 2 * s, 0, 0};
    const Vec4 az{0, 0, 2 * s, 0};
    const Vec4 ak{0, 0, 0, 2 * s};
    /* 8 cubic cells: ± each 4D axis. */
    tetCube(o, ax, ay, az, tets);             /* k = -s */
    tetCube(o + ak, ax, ay, az, tets);        /* k = +s */
    tetCube(o, ax, ay, ak, tets);             /* z = -s */
    tetCube(o + az, ax, ay, ak, tets);        /* z = +s */
    tetCube(o, ax, az, ak, tets);             /* y = -s */
    tetCube(o + ay, ax, az, ak, tets);        /* y = +s */
    tetCube(o, ay, az, ak, tets);             /* x = -s */
    tetCube(o + ax, ay, az, ak, tets);        /* x = +s */
}

void build5CellTets(double size, std::vector<Tet4>& tets)
{
    tets.clear();
    const double s = std::abs(size);
    /* Regular 5-cell (simplex) vertices. */
    const Vec4 v0{s, s, s, -s / std::sqrt(5.0)};
    const Vec4 v1{s, -s, -s, -s / std::sqrt(5.0)};
    const Vec4 v2{-s, s, -s, -s / std::sqrt(5.0)};
    const Vec4 v3{-s, -s, s, -s / std::sqrt(5.0)};
    const Vec4 v4{0, 0, 0, s * 2.0 / std::sqrt(5.0)};
    tets.push_back({v0, v1, v2, v3});
    tets.push_back({v0, v1, v2, v4});
    tets.push_back({v0, v1, v3, v4});
    tets.push_back({v0, v2, v3, v4});
    tets.push_back({v1, v2, v3, v4});
}

void build16CellTets(double size, std::vector<Tet4>& tets)
{
    tets.clear();
    const double s = std::abs(size);
    Vec4 v[8] = {{s, 0, 0, 0}, {-s, 0, 0, 0}, {0, s, 0, 0}, {0, -s, 0, 0},
                 {0, 0, s, 0}, {0, 0, -s, 0}, {0, 0, 0, s}, {0, 0, 0, -s}};
    /* 16-cell cells are tets through opposite-sign axis verts. */
    const int cells[][4] = {
        {0, 2, 4, 6}, {0, 2, 4, 7}, {0, 2, 5, 6}, {0, 2, 5, 7}, {0, 3, 4, 6}, {0, 3, 4, 7}, {0, 3, 5, 6}, {0, 3, 5, 7},
        {1, 2, 4, 6}, {1, 2, 4, 7}, {1, 2, 5, 6}, {1, 2, 5, 7}, {1, 3, 4, 6}, {1, 3, 4, 7}, {1, 3, 5, 6}, {1, 3, 5, 7},
    };
    for (const auto& c : cells)
        tets.push_back({v[c[0]], v[c[1]], v[c[2]], v[c[3]]});
}

void tetsToEdges(const std::vector<Tet4>& tets, std::vector<Vec4>& verts, std::vector<Edge4D>& edges)
{
    verts.clear();
    edges.clear();
    auto addV = [&](const Vec4& p) {
        for (const Vec4& q : verts) {
            if ((q - p).len2() < 1e-12)
                return;
        }
        verts.push_back(p);
    };
    auto addE = [&](Vec4 a, Vec4 b) {
        edges.push_back({a, b});
    };
    for (const Tet4& t : tets) {
        addV(t.a);
        addV(t.b);
        addV(t.c);
        addV(t.d);
        addE(t.a, t.b);
        addE(t.a, t.c);
        addE(t.a, t.d);
        addE(t.b, t.c);
        addE(t.b, t.d);
        addE(t.c, t.d);
    }
}

} // namespace fourd
