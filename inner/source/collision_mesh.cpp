#include "collision_mesh.h"
#include "figures.h"
#include "manual_shapes.h"
#include "transform_wrapper.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static vec<> rotX(const vec<>& v, double deg)
{
    const double rad = deg * M_PI / 180.0;
    const double c = std::cos(rad), s = std::sin(rad);
    return vec<>(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
}

static vec<> rotY(const vec<>& v, double deg)
{
    const double rad = deg * M_PI / 180.0;
    const double c = std::cos(rad), s = std::sin(rad);
    return vec<>(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

static vec<> rotZ(const vec<>& v, double deg)
{
    const double rad = deg * M_PI / 180.0;
    const double c = std::cos(rad), s = std::sin(rad);
    return vec<>(c * v.x - s * v.y, s * v.x + c * v.y, v.z);
}

static void transformTris(std::vector<CollTri>& tris, const vec<>& scale, double rx, double ry, double rz, const vec<>& pos)
{
    for (CollTri& t : tris) {
        auto map = [&](const vec<>& lc) {
            vec<> p(lc.x * scale.x, lc.y * scale.y, lc.z * scale.z);
            p = rotX(p, rx);
            p = rotY(p, ry);
            p = rotZ(p, rz);
            return p + pos;
        };
        t.v0 = map(t.v0);
        t.v1 = map(t.v1);
        t.v2 = map(t.v2);
    }
}

vec<> CollTri::normal() const
{
    vec<> e1 = v1 - v0;
    vec<> e2 = v2 - v0;
    vec<> n = e1 ^ e2;
    const double l2 = n.len2();
    if (l2 < 1e-18)
        return vec<>(0, 1, 0);
    return n * (1.0 / std::sqrt(l2));
}

double CollTri::area() const
{
    return 0.5 * ((v1 - v0) ^ (v2 - v0)).len();
}

vec<> CollTri::centroid() const
{
    return (v0 + v1 + v2) * (1.0 / 3.0);
}

namespace collision {

bool gLodO1Enabled = false;

int maxSubdivForFaceSize(double faceSize)
{
    const double s = std::max(0.25, std::abs(faceSize));
    return std::clamp(static_cast<int>(std::ceil(s / 0.35)), 2, 20);
}

int lodFaceSubdiv(double faceSize, double distance)
{
    const int minDiv = 2;
    const int maxDiv = maxSubdivForFaceSize(faceSize);
    if (!gLodO1Enabled)
        return kFaceSubdiv;
    const double ref = std::max(1.0, std::abs(faceSize) * 6.0);
    const double t = std::clamp(distance / ref, 0.0, 1.0);
    const int div = static_cast<int>(std::lround(maxDiv + (minDiv - maxDiv) * t));
    return std::clamp(div, minDiv, maxDiv);
}

static void pushTri(const vec<>& a, const vec<>& b, const vec<>& c, std::vector<CollTri>& out)
{
    if (((b - a) ^ (c - a)).len2() < 1e-18)
        return;
    out.push_back({a, b, c});
}

static void appendQuad(const vec<>& p00, const vec<>& p10, const vec<>& p11, const vec<>& p01, int uDiv, int vDiv,
                       std::vector<CollTri>& out)
{
    uDiv = std::max(1, uDiv);
    vDiv = std::max(1, vDiv);
    for (int iu = 0; iu < uDiv; ++iu) {
        for (int iv = 0; iv < vDiv; ++iv) {
            const double fu0 = static_cast<double>(iu) / uDiv;
            const double fu1 = static_cast<double>(iu + 1) / uDiv;
            const double fv0 = static_cast<double>(iv) / vDiv;
            const double fv1 = static_cast<double>(iv + 1) / vDiv;
            auto corner = [&](double fu, double fv) {
                const vec<> top = p00 + (p10 - p00) * fu;
                const vec<> bot = p01 + (p11 - p01) * fu;
                return top + (bot - top) * fv;
            };
            const vec<> c00 = corner(fu0, fv0);
            const vec<> c10 = corner(fu1, fv0);
            const vec<> c11 = corner(fu1, fv1);
            const vec<> c01 = corner(fu0, fv1);
            pushTri(c00, c10, c11, out);
            pushTri(c00, c11, c01, out);
        }
    }
}

void appendBoxTriangles(double hx, double hy, double hz, int faceSubdiv, std::vector<CollTri>& out)
{
    const double x = std::abs(hx);
    const double y = std::abs(hy);
    const double z = std::abs(hz);
    appendQuad(vec<>(-x, -y, z), vec<>(x, -y, z), vec<>(x, y, z), vec<>(-x, y, z), faceSubdiv, faceSubdiv, out);
    appendQuad(vec<>(x, -y, -z), vec<>(-x, -y, -z), vec<>(-x, y, -z), vec<>(x, y, -z), faceSubdiv, faceSubdiv, out);
    appendQuad(vec<>(-x, y, -z), vec<>(-x, y, z), vec<>(x, y, z), vec<>(x, y, -z), faceSubdiv, faceSubdiv, out);
    appendQuad(vec<>(-x, -y, -z), vec<>(x, -y, -z), vec<>(x, -y, z), vec<>(-x, -y, z), faceSubdiv, faceSubdiv, out);
    appendQuad(vec<>(x, -y, -z), vec<>(x, -y, z), vec<>(x, y, z), vec<>(x, y, -z), faceSubdiv, faceSubdiv, out);
    appendQuad(vec<>(-x, -y, -z), vec<>(-x, -y, z), vec<>(-x, y, z), vec<>(-x, y, -z), faceSubdiv, faceSubdiv, out);
}

void appendPyramidTriangles(double baseHalf, double height, std::vector<CollTri>& out)
{
    const double a = std::abs(baseHalf);
    const double h = std::abs(height);
    const double hb = h * 0.5;
    const vec<> apex(0, hb, 0);
    const vec<> bl(-a, -hb, -a);
    const vec<> br(a, -hb, -a);
    const vec<> fr(a, -hb, a);
    const vec<> fl(-a, -hb, a);
    pushTri(bl, br, apex, out);
    pushTri(br, fr, apex, out);
    pushTri(fr, fl, apex, out);
    pushTri(fl, bl, apex, out);
    pushTri(bl, br, fr, out);
    pushTri(bl, fr, fl, out);
}

vec<> closestPointOnTriangle(const vec<>& p, const vec<>& a, const vec<>& b, const vec<>& c)
{
    const vec<> ab = b - a;
    const vec<> ac = c - a;
    const vec<> ap = p - a;
    const double d1 = ab.dot(ap);
    const double d2 = ac.dot(ap);
    if (d1 <= 0.0 && d2 <= 0.0)
        return a;

    const vec<> bp = p - b;
    const double d3 = ab.dot(bp);
    const double d4 = ac.dot(bp);
    if (d3 >= 0.0 && d4 <= d3)
        return b;

    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double v = d1 / (d1 - d3);
        return a + ab * v;
    }

    const vec<> cp = p - c;
    const double d5 = ab.dot(cp);
    const double d6 = ac.dot(cp);
    if (d6 >= 0.0 && d5 <= d6)
        return c;

    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double w = d2 / (d2 - d6);
        return a + ac * w;
    }

    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w;
    }

    const double denom = 1.0 / (va + vb + vc);
    const double v = vb * denom;
    const double w = vc * denom;
    return a + ab * v + ac * w;
}

bool sphereTriangleContact(const vec<>& center, double radius, const CollTri& tri, CollisionContact& out)
{
    const vec<> p = closestPointOnTriangle(center, tri.v0, tri.v1, tri.v2);
    vec<> d = center - p;
    const double dist = d.len();
    if (dist > radius)
        return false;
    vec<> n = tri.normal();
    if (n.dot(d) < 0.0)
        n = n * -1.0;
    if (n.len2() < 1e-18)
        n = vec<>(0, 1, 0);
    out.normal = n;
    out.penetration = radius - dist;
    out.point = p;
    return out.penetration > 0.0;
}

bool bestSphereTriangleContact(const vec<>& center, double radius, const std::vector<CollTri>& tris,
                               CollisionContact& out, const vec<>* meshCenter)
{
    CollisionContact best;
    best.penetration = -1.0;
    bool hit = false;
    for (const CollTri& tri : tris) {
        CollisionContact tmp;
        if (!sphereTriangleContact(center, radius, tri, tmp))
            continue;
        const vec<> sep = center - tmp.point;
        if (tmp.normal.dot(sep) < 0.02)
            continue;
        if (meshCenter) {
            const vec<> up = center - *meshCenter;
            if (up.y > 0.15 && tmp.normal.y < 0.35)
                continue;
            if (up.y < -0.15 && tmp.normal.y > -0.35)
                continue;
        }
        if (tmp.penetration > best.penetration) {
            best = tmp;
            hit = true;
        }
    }
    if (!hit)
        return false;
    out = best;
    return true;
}

bool sphereAabbContact(const vec<>& center, double radius, const vec<>& boxCenter, const vec<>& halfExtents,
                       CollisionContact& out)
{
    const vec<> mn(boxCenter.x - halfExtents.x, boxCenter.y - halfExtents.y, boxCenter.z - halfExtents.z);
    const vec<> mx(boxCenter.x + halfExtents.x, boxCenter.y + halfExtents.y, boxCenter.z + halfExtents.z);
    const vec<> q(std::clamp(center.x, mn.x, mx.x), std::clamp(center.y, mn.y, mx.y), std::clamp(center.z, mn.z, mx.z));
    const vec<> v = center - q;
    const double d2 = v.len2();
    if (d2 > radius * radius)
        return false;
    const double d = std::sqrt(std::max(0.0, d2));
    out.normal = d > 1e-8 ? v * (1.0 / d) : vec<>(0, 1, 0);
    out.penetration = radius - d;
    out.point = q;
    return out.penetration > 0.0;
}

static bool sphereOverlapsPlateXZ(const vec<>& center, double radius, const vec<>& boxCenter,
                                  const vec<>& halfExtents)
{
    const double dx = std::max(0.0, std::abs(center.x - boxCenter.x) - halfExtents.x);
    const double dz = std::max(0.0, std::abs(center.z - boxCenter.z) - halfExtents.z);
    return dx * dx + dz * dz <= radius * radius;
}

bool sphereThinPlateTopContact(const vec<>& center, double radius, const vec<>& boxCenter,
                               const vec<>& halfExtents, CollisionContact& out)
{
    if (!sphereOverlapsPlateXZ(center, radius, boxCenter, halfExtents))
        return false;
    const double topY = boxCenter.y + halfExtents.y;
    const double bottom = center.y - radius;
    if (bottom > topY + 1e-6)
        return false;
    const double pen = topY - bottom;
    if (pen <= 1e-9)
        return false;
    out.normal = vec<>(0, 1, 0);
    out.point = vec<>(center.x, topY, center.z);
    out.penetration = pen;
    return true;
}

bool sphereThinPlateTopSwept(const vec<>& p0, const vec<>& p1, double radius, const vec<>& boxCenter,
                             const vec<>& halfExtents, CollisionContact& out)
{
    if (!sphereOverlapsPlateXZ(p1, radius, boxCenter, halfExtents) &&
        !sphereOverlapsPlateXZ(p0, radius, boxCenter, halfExtents))
        return false;
    const double topY = boxCenter.y + halfExtents.y;
    const double b0 = p0.y - radius;
    const double b1 = p1.y - radius;
    if (b0 > topY && b1 > topY)
        return false;
    if (b1 <= topY) {
        return sphereThinPlateTopContact(p1, radius, boxCenter, halfExtents, out);
    }
    const double denom = b1 - b0;
    if (std::abs(denom) < 1e-12)
        return false;
    const double t = std::clamp((topY - b0) / denom, 0.0, 1.0);
    const vec<> hit = p0 + (p1 - p0) * t;
    out.normal = vec<>(0, 1, 0);
    out.point = vec<>(hit.x, topY, hit.z);
    out.penetration = std::max(1e-4, (p1.y + radius) - topY);
    return true;
}

void appendSphereTriangles(double radius, int slices, int stacks, std::vector<CollTri>& out)
{
    const double r = std::abs(radius);
    slices = std::clamp(slices, 6, 32);
    stacks = std::clamp(stacks, 4, 24);
    for (int i = 0; i < stacks; ++i) {
        const double v0 = M_PI * (static_cast<double>(i) / stacks - 0.5);
        const double v1 = M_PI * (static_cast<double>(i + 1) / stacks - 0.5);
        const double y0 = r * std::sin(v0);
        const double y1 = r * std::sin(v1);
        const double r0 = r * std::cos(v0);
        const double r1 = r * std::cos(v1);
        for (int j = 0; j < slices; ++j) {
            const double u0 = 2.0 * M_PI * static_cast<double>(j) / slices;
            const double u1 = 2.0 * M_PI * static_cast<double>(j + 1) / slices;
            const vec<> p00(r0 * std::cos(u0), y0, r0 * std::sin(u0));
            const vec<> p10(r0 * std::cos(u1), y0, r0 * std::sin(u1));
            const vec<> p01(r1 * std::cos(u0), y1, r1 * std::sin(u0));
            const vec<> p11(r1 * std::cos(u1), y1, r1 * std::sin(u1));
            pushTri(p00, p10, p11, out);
            pushTri(p00, p11, p01, out);
        }
    }
}

void appendConeTriangles(double radius, double height, int segments, std::vector<CollTri>& out)
{
    const double r = std::abs(radius);
    const double h = std::abs(height);
    segments = std::clamp(segments, 6, 32);
    const vec<> apex(0, h * 0.5, 0);
    const double yb = -h * 0.5;
    for (int i = 0; i < segments; ++i) {
        const double a0 = 2.0 * M_PI * static_cast<double>(i) / segments;
        const double a1 = 2.0 * M_PI * static_cast<double>(i + 1) / segments;
        const vec<> p0(r * std::cos(a0), yb, r * std::sin(a0));
        const vec<> p1(r * std::cos(a1), yb, r * std::sin(a1));
        pushTri(p0, p1, apex, out);
        pushTri(vec<>(0, yb, 0), p1, p0, out);
    }
}

void appendTorusTriangles(double tubeRadius, double ringRadius, int sides, int rings, std::vector<CollTri>& out)
{
    /* Совпадает с glutSolidTorus(inner=tube, outer=major): кольцо в плоскости XZ. */
    const double tube = std::max(0.05, std::abs(tubeRadius));
    const double major = std::max(tube + 0.05, std::abs(ringRadius));
    sides = std::clamp(sides, 8, 32);
    rings = std::clamp(rings, 8, 32);
    /* Как glutSolidTorus: кольцо в плоскости XY, трубка по Z. */
    auto surface = [&](double u, double v) {
        const double cu = std::cos(u);
        const double su = std::sin(u);
        const double cv = std::cos(v);
        const double sv = std::sin(v);
        const double w = major + tube * cv;
        return vec<>(w * cu, w * su, tube * sv);
    };
    for (int i = 0; i < rings; ++i) {
        const double u0 = 2.0 * M_PI * static_cast<double>(i) / rings;
        const double u1 = 2.0 * M_PI * static_cast<double>(i + 1) / rings;
        for (int j = 0; j < sides; ++j) {
            const double v0 = 2.0 * M_PI * static_cast<double>(j) / sides;
            const double v1 = 2.0 * M_PI * static_cast<double>(j + 1) / sides;
            const vec<> p00 = surface(u0, v0);
            const vec<> p01 = surface(u0, v1);
            const vec<> p10 = surface(u1, v0);
            const vec<> p11 = surface(u1, v1);
            pushTri(p00, p10, p11, out);
            pushTri(p00, p11, p01, out);
        }
    }
}

void appendCylinderTriangles(double radius, double height, int slices, std::vector<CollTri>& out)
{
    const double r = std::abs(radius);
    const double hh = 0.5 * std::abs(height);
    slices = std::clamp(slices, 6, 32);
    for (int i = 0; i < slices; ++i) {
        const double a0 = 2.0 * M_PI * static_cast<double>(i) / slices;
        const double a1 = 2.0 * M_PI * static_cast<double>(i + 1) / slices;
        const vec<> b0(r * std::cos(a0), -hh, r * std::sin(a0));
        const vec<> b1(r * std::cos(a1), -hh, r * std::sin(a1));
        const vec<> t0(r * std::cos(a0), hh, r * std::sin(a0));
        const vec<> t1(r * std::cos(a1), hh, r * std::sin(a1));
        pushTri(b0, b1, t1, out);
        pushTri(b0, t1, t0, out);
        pushTri(vec<>(0, -hh, 0), b1, b0, out);
        pushTri(vec<>(0, hh, 0), t0, t1, out);
    }
}

static bool vertexPenetratesTriangle(const vec<>& v, const CollTri& tri, CollisionContact& out)
{
    const vec<> p = closestPointOnTriangle(v, tri.v0, tri.v1, tri.v2);
    const vec<> toV = v - p;
    const double planar = toV.len();
    const vec<> n = tri.normal();
    /* Глубина вдоль внутренней нормали; отсекаем «сквозь бесконечную плоскость» вдали от грани. */
    const double depth = -(n.dot(toV));
    if (depth <= 1e-5)
        return false;
    const double edge = std::sqrt(std::max(1e-9, tri.area()));
    if (planar > std::max(0.1, 0.4 * edge))
        return false;
    out.normal = n;
    out.penetration = depth;
    out.point = p;
    return true;
}

bool triangleTriangleContact(const CollTri& a, const CollTri& b, CollisionContact& out)
{
    CollisionContact best;
    best.penetration = -1.0;
    bool hit = false;
    auto tryV = [&](const vec<>& v, const CollTri& tri) {
        CollisionContact tmp;
        if (vertexPenetratesTriangle(v, tri, tmp) && tmp.penetration > best.penetration) {
            best = tmp;
            hit = true;
        }
    };
    tryV(a.v0, b);
    tryV(a.v1, b);
    tryV(a.v2, b);
    tryV(b.v0, a);
    tryV(b.v1, a);
    tryV(b.v2, a);
    if (!hit)
        return false;
    out = best;
    return true;
}

bool meshBodyOnThinPlateTop(const std::vector<CollTri>& bodyTris, const vec<>& plateCenter,
                            const vec<>& plateHalfExtents, CollisionContact& out)
{
    const double topY = plateCenter.y + plateHalfExtents.y;
    const double minX = plateCenter.x - plateHalfExtents.x;
    const double maxX = plateCenter.x + plateHalfExtents.x;
    const double minZ = plateCenter.z - plateHalfExtents.z;
    const double maxZ = plateCenter.z + plateHalfExtents.z;
    double lowest = 1e30;
    bool any = false;
    for (const CollTri& tri : bodyTris) {
        for (const vec<>* vp : {&tri.v0, &tri.v1, &tri.v2}) {
            if (vp->x < minX || vp->x > maxX || vp->z < minZ || vp->z > maxZ)
                continue;
            lowest = std::min(lowest, vp->y);
            any = true;
        }
    }
    if (!any || lowest > topY + 1e-4)
        return false;
    const double pen = topY - lowest;
    if (pen <= 1e-9)
        return false;
    out.normal = vec<>(0, 1, 0);
    out.point = vec<>(plateCenter.x, topY, plateCenter.z);
    out.penetration = pen;
    return true;
}

bool buildObjectCollisionMesh(based* obj, std::vector<CollTri>& out, int faceSubdiv)
{
    out.clear();
    if (!obj)
        return false;
    faceSubdiv = std::max(1, faceSubdiv);

    if (auto* w = dynamic_cast<TransformWrapper*>(obj)) {
        if (!w->getChild())
            return false;
        std::vector<CollTri> local;
        if (!buildObjectCollisionMesh(w->getChild(), local, faceSubdiv))
            return false;
        transformTris(local, w->getScale(), w->getRx(), w->getRy(), w->getRz(), w->getPos());
        out = std::move(local);
        return !out.empty();
    }

    if (auto* bx = dynamic_cast<EditorBox*>(obj)) {
        appendBoxTriangles(0.5 * std::abs(bx->dx) * std::abs(bx->scale.x),
                           0.5 * std::abs(bx->dy) * std::abs(bx->scale.y),
                           0.5 * std::abs(bx->dz) * std::abs(bx->scale.z), faceSubdiv, out);
        transformTris(out, vec<>(1, 1, 1), bx->rx, bx->ry, bx->rz, bx->pos);
        return true;
    }
    if (auto* es = dynamic_cast<EditorSphere*>(obj)) {
        appendSphereTriangles(std::abs(es->radius), rs::ed_sph_slc, rs::ed_sph_stk, out);
        transformTris(out, es->scale, es->rx, es->ry, es->rz, es->pos);
        return true;
    }
    if (auto* ec = dynamic_cast<EditorCylinder*>(obj)) {
        appendCylinderTriangles(std::abs(ec->baseRadius), std::abs(ec->height), std::max(6, rs::ed_cyl_slc), out);
        transformTris(out, ec->scale, ec->rx, ec->ry, ec->rz, ec->pos);
        return true;
    }
    if (auto* sc = dynamic_cast<SolidCube*>(obj)) {
        appendBoxTriangles(sc->hx, sc->hy, sc->hz, faceSubdiv, out);
        return true;
    }
    if (auto* py = dynamic_cast<SolidPyramid*>(obj)) {
        appendPyramidTriangles(0.5 * std::abs(py->base), std::abs(py->height), out);
        return true;
    }
    if (auto* co = dynamic_cast<SolidCone*>(obj)) {
        appendConeTriangles(co->radius, co->height, std::max(6, rs::cone_seg), out);
        return true;
    }
    if (auto* cyl = dynamic_cast<SolidCylinder*>(obj)) {
        appendCylinderTriangles(cyl->radius, cyl->height, std::max(6, rs::ed_cyl_slc), out);
        return true;
    }
    if (auto* sp = dynamic_cast<SolidSphere*>(obj)) {
        appendSphereTriangles(sp->radius, rs::ed_sph_slc, rs::ed_sph_stk, out);
        return true;
    }
    if (auto* to = dynamic_cast<EditorTorus*>(obj)) {
        appendTorusTriangles(std::abs(to->innerR), std::abs(to->outerR), rs::ed_tor_s, rs::ed_tor_r, out);
        transformTris(out, to->scale, to->rx, to->ry, to->rz, to->pos);
        return true;
    }
    if (auto* st = dynamic_cast<SolidTorus*>(obj)) {
        appendTorusTriangles(st->innerR, st->outerR, rs::ed_tor_s, rs::ed_tor_r, out);
        return true;
    }
    return false;
}

void buildWorldCollisionMesh(based* obj, const vec<>& bodyCenter, const vec<>& pivotCenter, const vec<>& spinAxis,
                             double spinDeg, int faceSubdiv, std::vector<CollTri>& out)
{
    out.clear();
    std::vector<CollTri> placed;
    if (!buildObjectCollisionMesh(obj, placed, faceSubdiv) || placed.empty())
        return;

    const double spinRad = spinDeg * M_PI / 180.0;
    auto map = [&](const vec<>& p) {
        vec<> rel = p - pivotCenter;
        if (std::abs(spinRad) > 1e-12) {
            const double w = spinAxis.len();
            if (w > 1e-12) {
                const vec<> ax = spinAxis * (1.0 / w);
                const double c = std::cos(spinRad), s = std::sin(spinRad);
                rel = rel * c + (ax ^ rel) * s + ax * (ax.dot(rel) * (1.0 - c));
            }
        }
        return bodyCenter + rel;
    };

    out.reserve(placed.size());
    for (const CollTri& t : placed) {
        out.push_back({map(t.v0), map(t.v1), map(t.v2)});
    }
}

} // namespace collision
