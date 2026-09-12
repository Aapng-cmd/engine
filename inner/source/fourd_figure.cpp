#include "fourd_figure.h"
#include "engine_power.h"
#include "transform_wrapper.h"

#include <algorithm>
#include <cmath>

FourDWireFigure::FourDWireFigure(const std::string& type, vec<> p, vec<> s, double rx_, double ry_, double rz_,
                                 double size, vec<> col, GLuint tex)
    : shapeType(type), pos(p), scale(s), rx(rx_), ry(ry_), rz(rz_), sizeParam(size), color(col)
{
    textureID = tex;
    rebuildGeometry();
}

void FourDWireFigure::rebuildGeometry()
{
    tets.clear();
    verts4.clear();
    edges4.clear();
    if (shapeType == "tesseract") {
        fourd::buildTesseract(sizeParam, verts4, edges4);
        fourd::buildTesseractTets(sizeParam, tets);
    } else if (shapeType == "hypersphere") {
        fourd::buildHypersphereWire(sizeParam, engine::hypersphereSlices(), engine::hypersphereStacks(), verts4,
                                    edges4);
        fourd::build16CellTets(sizeParam, tets);
    } else if (shapeType == "pyramid4d") {
        fourd::build5CellTets(sizeParam, tets);
        fourd::tetsToEdges(tets, verts4, edges4);
    } else if (shapeType == "16cell") {
        fourd::build16CellTets(sizeParam, tets);
        fourd::tetsToEdges(tets, verts4, edges4);
    }
}

void FourDWireFigure::collectSliceTris(const vec<>& worldPos, double kOffset, double sliceK,
                                       std::vector<SliceTri>& out) const
{
    out.clear();
    SliceTri tmp[2];
    for (const Tet4& tet : tets) {
        Tet4 w;
        w.a = fourd::transformLocal4D(tet.a, worldPos, scale, rx, ry, rz, kOffset, rwx, rwy, rwz);
        w.b = fourd::transformLocal4D(tet.b, worldPos, scale, rx, ry, rz, kOffset, rwx, rwy, rwz);
        w.c = fourd::transformLocal4D(tet.c, worldPos, scale, rx, ry, rz, kOffset, rwx, rwy, rwz);
        w.d = fourd::transformLocal4D(tet.d, worldPos, scale, rx, ry, rz, kOffset, rwx, rwy, rwz);
        const int n = fourd::sliceTet(w, sliceK, tmp);
        for (int i = 0; i < n; ++i)
            out.push_back(tmp[i]);
    }
}

void FourDWireFigure::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    const double m = std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
    out.push_back({pos, std::abs(sizeParam) * m * 1.8});
}

void FourDWireFigure::drawSliced(double sliceK, double kWorld, const vec<>* worldPos) const
{
    const vec<>& wp = worldPos ? *worldPos : pos;
    std::vector<SliceTri> tris;
    collectSliceTris(wp, kWorld, sliceK, tris);
    glColor4d(color.x, color.y, color.z, renderAlpha);
    glBegin(GL_TRIANGLES);
    for (const SliceTri& t : tris) {
        const vec<> n = !((t.v1 - t.v0) ^ (t.v2 - t.v0));
        glNormal3d(n.x, n.y, n.z);
        glVertex3d(t.v0.x, t.v0.y, t.v0.z);
        glVertex3d(t.v1.x, t.v1.y, t.v1.z);
        glVertex3d(t.v2.x, t.v2.y, t.v2.z);
    }
    glEnd();
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    glColor4d(color.x * 0.35, color.y * 0.35, color.z * 0.45, renderAlpha);
    glBegin(GL_LINES);
    for (const SliceTri& t : tris) {
        glVertex3d(t.v0.x, t.v0.y, t.v0.z);
        glVertex3d(t.v1.x, t.v1.y, t.v1.z);
        glVertex3d(t.v1.x, t.v1.y, t.v1.z);
        glVertex3d(t.v2.x, t.v2.y, t.v2.z);
        glVertex3d(t.v2.x, t.v2.y, t.v2.z);
        glVertex3d(t.v0.x, t.v0.y, t.v0.z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void FourDWireFigure::drawProjected(const Camera4DState& cam, double kWorld, const vec<>* worldPos) const
{
    const vec<>& wp = worldPos ? *worldPos : pos;
    glColor4d(color.x, color.y, color.z, renderAlpha);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (const Edge4D& e : edges4) {
        const Vec4 wa = fourd::transformLocal4D(e.a, wp, scale, rx, ry, rz, kWorld, rwx, rwy, rwz);
        const Vec4 wb = fourd::transformLocal4D(e.b, wp, scale, rx, ry, rz, kWorld, rwx, rwy, rwz);
        vec<> a3, b3;
        if (!fourd::projectTo3D(cam, wa, a3) || !fourd::projectTo3D(cam, wb, b3))
            continue;
        glVertex3d(a3.x, a3.y, a3.z);
        glVertex3d(b3.x, b3.y, b3.z);
    }
    glEnd();
}

void FourDWireFigure::Draw(double /*t*/)
{
    drawSliced(kPos, kPos, nullptr);
}

void FourDWireFigure::uniqueLocalVerts(std::vector<Vec4>& out) const
{
    out.clear();
    auto add = [&](const Vec4& p) {
        for (const Vec4& q : out) {
            if ((q - p).len2() < 1e-12)
                return;
        }
        out.push_back(p);
    };
    for (const Tet4& t : tets) {
        add(t.a);
        add(t.b);
        add(t.c);
        add(t.d);
    }
}

Vec4 FourDWireFigure::worldVert(const Vec4& local, const vec<>& worldPos, double kOffset) const
{
    return fourd::transformLocal4D(local, worldPos, scale, rx, ry, rz, kOffset, rwx, rwy, rwz);
}

bool FourDWireFigure::moveUniqueVert(int index, const Vec4& local)
{
    std::vector<Vec4> uniq;
    uniqueLocalVerts(uniq);
    if (index < 0 || index >= static_cast<int>(uniq.size()))
        return false;
    const Vec4 old = uniq[static_cast<size_t>(index)];
    auto upd = [&](Vec4& p) {
        if ((p - old).len2() < 1e-12)
            p = local;
    };
    for (Tet4& t : tets) {
        upd(t.a);
        upd(t.b);
        upd(t.c);
        upd(t.d);
    }
    fourd::tetsToEdges(tets, verts4, edges4);
    return true;
}

bool FourDWireFigure::moveUniqueVertOnSlice(int index, const vec<>& xyz, double sliceK, const vec<>& worldPos,
                                            double kOffset)
{
    const Vec4 world{xyz.x, xyz.y, xyz.z, sliceK};
    const Vec4 local = fourd::inverseTransformLocal4D(world, worldPos, scale, rx, ry, rz, kOffset, rwx, rwy, rwz);
    return moveUniqueVert(index, local);
}

void applyFourDAngles(based* o, double rwx, double rwy, double rwz)
{
    FourDWireFigure* f = nullptr;
    if (auto* w = dynamic_cast<TransformWrapper*>(o))
        f = dynamic_cast<FourDWireFigure*>(w->getChild());
    else
        f = dynamic_cast<FourDWireFigure*>(o);
    if (!f)
        return;
    f->rwx = rwx;
    f->rwy = rwy;
    f->rwz = rwz;
}

FourDWireFigure* asFourDFigure(based* o)
{
    if (auto* w = dynamic_cast<TransformWrapper*>(o))
        return dynamic_cast<FourDWireFigure*>(w->getChild());
    return dynamic_cast<FourDWireFigure*>(o);
}

const FourDWireFigure* asFourDFigure(const based* o)
{
    if (auto* w = dynamic_cast<const TransformWrapper*>(o))
        return dynamic_cast<const FourDWireFigure*>(w->getChild());
    return dynamic_cast<const FourDWireFigure*>(o);
}

void applyFourDTets(based* o, const std::vector<Tet4>& tets)
{
    FourDWireFigure* f = asFourDFigure(o);
    if (!f || tets.empty())
        return;
    f->tets = tets;
    if (static_cast<int>(f->tets.size()) > kMaxFourDTets)
        f->tets.resize(static_cast<size_t>(kMaxFourDTets));
    fourd::tetsToEdges(f->tets, f->verts4, f->edges4);
}

void copyFourDTets(const based* o, std::vector<Tet4>& tets)
{
    tets.clear();
    const FourDWireFigure* f = asFourDFigure(o);
    if (!f)
        return;
    tets = f->tets;
}

void packFourDTets(const std::vector<Tet4>& tets, std::vector<double>& packed)
{
    packed.clear();
    const int n = std::min(static_cast<int>(tets.size()), kMaxFourDTets);
    packed.reserve(static_cast<size_t>(n) * 16);
    for (int i = 0; i < n; ++i) {
        const Tet4& t = tets[static_cast<size_t>(i)];
        const Vec4 vs[4] = {t.a, t.b, t.c, t.d};
        for (const Vec4& v : vs) {
            packed.push_back(v.x);
            packed.push_back(v.y);
            packed.push_back(v.z);
            packed.push_back(v.k);
        }
    }
}

bool unpackFourDTets(const std::vector<double>& packed, std::vector<Tet4>& tets)
{
    tets.clear();
    if (packed.size() < 16 || packed.size() % 16 != 0)
        return false;
    int n = static_cast<int>(packed.size() / 16);
    if (n > kMaxFourDTets)
        n = kMaxFourDTets;
    tets.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const double* p = packed.data() + static_cast<size_t>(i) * 16;
        tets.push_back(Tet4{{p[0], p[1], p[2], p[3]},
                            {p[4], p[5], p[6], p[7]},
                            {p[8], p[9], p[10], p[11]},
                            {p[12], p[13], p[14], p[15]}});
    }
    return true;
}
