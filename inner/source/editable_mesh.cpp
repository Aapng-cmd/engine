#include "editable_mesh.h"
#include "transform_wrapper.h"
#include <cmath>

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

static vec<> mapLocal(const vec<>& lc, const vec<>& scale, double rx, double ry, double rz, const vec<>& pos)
{
    vec<> p(lc.x * scale.x, lc.y * scale.y, lc.z * scale.z);
    p = rotX(p, rx);
    p = rotY(p, ry);
    p = rotZ(p, rz);
    return p + pos;
}

void EditableMesh::fillUnitCube()
{
    verts = {vec<>(-0.5, -0.5, -0.5), vec<>(0.5, -0.5, -0.5), vec<>(0.5, 0.5, -0.5), vec<>(-0.5, 0.5, -0.5),
             vec<>(-0.5, -0.5, 0.5),  vec<>(0.5, -0.5, 0.5),  vec<>(0.5, 0.5, 0.5),  vec<>(-0.5, 0.5, 0.5)};
    indices = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 4, 5, 0, 5, 1,
               3, 2, 6, 3, 6, 7, 0, 3, 7, 0, 7, 4, 1, 5, 6, 1, 6, 2};
}

void EditableMesh::fillPlane(double hx, double hz)
{
    const double x = std::abs(hx) * 0.5;
    const double z = std::abs(hz) * 0.5;
    verts = {vec<>(-x, 0, -z), vec<>(x, 0, -z), vec<>(x, 0, z), vec<>(-x, 0, z)};
    indices = {0, 1, 2, 0, 2, 3};
}

void EditableMesh::setVertex(int i, const vec<>& p)
{
    if (i >= 0 && i < vertCount())
        verts[static_cast<size_t>(i)] = p;
}

void EditableMesh::triangleUv(const vec<>& a, const vec<>& b, const vec<>& c, double uv[3][2])
{
    vec<> n = !((b - a) ^ (c - a));
    vec<> t = (std::abs(n.y) < 0.9) ? !(n ^ vec<>(0, 1, 0)) : !(n ^ vec<>(1, 0, 0));
    vec<> bit = !(n ^ t);
    auto proj = [&](const vec<>& p, double& u, double& v) {
        u = p.dot(t);
        v = p.dot(bit);
    };
    proj(a, uv[0][0], uv[0][1]);
    proj(b, uv[1][0], uv[1][1]);
    proj(c, uv[2][0], uv[2][1]);
}

bool EditableMesh::fillFromWorldTris(const std::vector<CollTri>& tris)
{
    verts.clear();
    indices.clear();
    auto toLocal = [&](const vec<>& w) {
        vec<> p = w - pos;
        p = rotZ(p, -rz);
        p = rotY(p, -ry);
        p = rotX(p, -rx);
        const double sx = std::abs(scale.x) > 1e-12 ? scale.x : 1.0;
        const double sy = std::abs(scale.y) > 1e-12 ? scale.y : 1.0;
        const double sz = std::abs(scale.z) > 1e-12 ? scale.z : 1.0;
        return vec<>(p.x / sx, p.y / sy, p.z / sz);
    };
    auto weld = [&](const vec<>& p) {
        for (int i = 0; i < vertCount(); ++i) {
            if ((verts[static_cast<size_t>(i)] - p).len2() < 1e-10)
                return i;
        }
        if (!canAddVerts(1))
            return -1;
        verts.push_back(p);
        return vertCount() - 1;
    };
    for (const CollTri& t : tris) {
        if (!canAddTris(1))
            break;
        const int i0 = weld(toLocal(t.v0));
        const int i1 = weld(toLocal(t.v1));
        const int i2 = weld(toLocal(t.v2));
        if (i0 < 0 || i1 < 0 || i2 < 0)
            break;
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);
    }
    return triCount() > 0;
}

bool EditableMesh::extrudeFace(int face, double amount)
{
    if (face < 0 || face >= triCount())
        return false;
    if (!canAddVerts(3) || !canAddTris(7))
        return false;
    const int i0 = indices[static_cast<size_t>(face * 3 + 0)];
    const int i1 = indices[static_cast<size_t>(face * 3 + 1)];
    const int i2 = indices[static_cast<size_t>(face * 3 + 2)];
    const vec<> a = verts[static_cast<size_t>(i0)];
    const vec<> b = verts[static_cast<size_t>(i1)];
    const vec<> c = verts[static_cast<size_t>(i2)];
    vec<> n = (b - a) ^ (c - a);
    const double l = n.len();
    if (l < 1e-12)
        return false;
    n = n * (amount / l);
    const int n0 = vertCount();
    verts.push_back(a + n);
    verts.push_back(b + n);
    verts.push_back(c + n);
    /* sides */
    auto quad = [&](int a0, int a1, int b1, int b0) {
        indices.push_back(a0);
        indices.push_back(a1);
        indices.push_back(b1);
        indices.push_back(a0);
        indices.push_back(b1);
        indices.push_back(b0);
    };
    quad(i0, i1, n0 + 1, n0);
    quad(i1, i2, n0 + 2, n0 + 1);
    quad(i2, i0, n0, n0 + 2);
    indices[static_cast<size_t>(face * 3 + 0)] = n0;
    indices[static_cast<size_t>(face * 3 + 1)] = n0 + 1;
    indices[static_cast<size_t>(face * 3 + 2)] = n0 + 2;
    return true;
}

vec<> EditableMesh::worldVertex(int i) const
{
    if (i < 0 || i >= vertCount())
        return pos;
    return mapLocal(verts[static_cast<size_t>(i)], scale, rx, ry, rz, pos);
}

void EditableMesh::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    if (verts.empty()) {
        out.push_back({pos, 0.5});
        return;
    }
    vec<> c(0, 0, 0);
    for (const vec<>& v : verts)
        c += mapLocal(v, scale, rx, ry, rz, pos);
    c = c * (1.0 / static_cast<double>(verts.size()));
    double r = 0.1;
    for (const vec<>& v : verts)
        r = std::max(r, (mapLocal(v, scale, rx, ry, rz, pos) - c).len());
    out.push_back({c, r});
}

void EditableMesh::drawLocal(double /*t*/)
{
    glPushMatrix();
    glRotated(rz, 0, 0, 1);
    glRotated(ry, 0, 1, 0);
    glRotated(rx, 1, 0, 0);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, renderAlpha);
    } else {
        glColor4d(color.x, color.y, color.z, renderAlpha);
    }
    glBegin(GL_TRIANGLES);
    for (size_t t = 0; t + 2 < indices.size(); t += 3) {
        const vec<>& a = verts[static_cast<size_t>(indices[t])];
        const vec<>& b = verts[static_cast<size_t>(indices[t + 1])];
        const vec<>& c = verts[static_cast<size_t>(indices[t + 2])];
        const vec<> n = !((b - a) ^ (c - a));
        double uv[3][2];
        triangleUv(a, b, c, uv);
        glNormal3d(n.x, n.y, n.z);
        glTexCoord2d(uv[0][0], uv[0][1]);
        glVertex3d(a.x, a.y, a.z);
        glTexCoord2d(uv[1][0], uv[1][1]);
        glVertex3d(b.x, b.y, b.z);
        glTexCoord2d(uv[2][0], uv[2][1]);
        glVertex3d(c.x, c.y, c.z);
    }
    glEnd();
    if (textureID != 0)
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void EditableMesh::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    drawLocal(t);
    glPopMatrix();
}

void applyEditableMeshData(based* o, const std::vector<vec<>>& verts, const std::vector<int>& indices)
{
    EditableMesh* m = nullptr;
    if (auto* w = dynamic_cast<TransformWrapper*>(o))
        m = dynamic_cast<EditableMesh*>(w->getChild());
    else
        m = dynamic_cast<EditableMesh*>(o);
    if (!m)
        return;
    m->verts = verts;
    m->indices = indices;
    if (static_cast<int>(m->verts.size()) > engine::maxMeshVerts())
        m->verts.resize(static_cast<size_t>(engine::maxMeshVerts()));
    const int maxIdx = static_cast<int>(m->verts.size()) - 1;
    std::vector<int> clean;
    clean.reserve(m->indices.size());
    for (size_t i = 0; i + 2 < m->indices.size() && static_cast<int>(clean.size() / 3) < engine::maxMeshTris();
         i += 3) {
        const int a = m->indices[i], b = m->indices[i + 1], c = m->indices[i + 2];
        if (a >= 0 && b >= 0 && c >= 0 && a <= maxIdx && b <= maxIdx && c <= maxIdx) {
            clean.push_back(a);
            clean.push_back(b);
            clean.push_back(c);
        }
    }
    m->indices = std::move(clean);
}

void copyEditableMeshData(const based* o, std::vector<vec<>>& verts, std::vector<int>& indices)
{
    const EditableMesh* m = nullptr;
    if (auto* w = dynamic_cast<const TransformWrapper*>(o))
        m = dynamic_cast<const EditableMesh*>(w->getChild());
    else
        m = dynamic_cast<const EditableMesh*>(o);
    if (!m)
        return;
    verts = m->verts;
    indices = m->indices;
}
