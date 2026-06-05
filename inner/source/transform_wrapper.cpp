#include "transform_wrapper.h"
#include "render_material.h"
#include <algorithm>
#include <cmath>

static vec<> rotX(const vec<>& v, double deg)
{
    double rad = deg * M_PI / 180.0;
    double c = cos(rad), s = sin(rad);
    return vec<>(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
}

static vec<> rotY(const vec<>& v, double deg)
{
    double rad = deg * M_PI / 180.0;
    double c = cos(rad), s = sin(rad);
    return vec<>(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

static vec<> rotZ(const vec<>& v, double deg)
{
    double rad = deg * M_PI / 180.0;
    double c = cos(rad), s = sin(rad);
    return vec<>(c * v.x - s * v.y, s * v.x + c * v.y, v.z);
}

/** World point: pos + Rz * Ry * Rx * (scale * lc) — matches glTranslated; glRotated; glScaled. */
static vec<> worldPointFromLocal(const vec<>& lc, const vec<>& scale, double rx, double ry, double rz, const vec<>& pos)
{
    vec<> p(lc.x * scale.x, lc.y * scale.y, lc.z * scale.z);
    p = rotX(p, rx);
    p = rotY(p, ry);
    p = rotZ(p, rz);
    return p + pos;
}

/** World-space bounding radius for a local sphere (handles non-uniform scale + rotation). */
static double worldRadiusFromLocal(const vec<>& lc, double localR, const vec<>& scale, double rx, double ry,
                                   double rz, const vec<>& pos)
{
    const vec<> wc = worldPointFromLocal(lc, scale, rx, ry, rz, pos);
    double maxD = 0.0;
    for (int axis = 0; axis < 3; ++axis) {
        for (int sign = -1; sign <= 1; sign += 2) {
            vec<> lp = lc;
            if (axis == 0)
                lp.x += sign * localR;
            else if (axis == 1)
                lp.y += sign * localR;
            else
                lp.z += sign * localR;
            const vec<> wp = worldPointFromLocal(lp, scale, rx, ry, rz, pos);
            maxD = std::max(maxD, (wp - wc).len());
        }
    }
    return std::max(localR * std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)}), maxD);
}

void setFigureRenderAlpha(based* o, double a)
{
    if (!o)
        return;
    const AlphaReflect ar = decomposeAlphaReflect(a);
    o->renderAlpha = ar.opacity;
    o->reflectAmount = ar.reflect;
    if (auto* w = dynamic_cast<TransformWrapper*>(o))
        setFigureRenderAlpha(w->getChild(), a);
}

TransformWrapper::TransformWrapper(based* owned, vec<> pos, vec<> scale, double rx, double ry, double rz)
    : child(owned), pos(pos), scale(scale), rx(rx), ry(ry), rz(rz)
{
    textureID = 0;
}

void TransformWrapper::drawLocal(double t)
{
    glPushMatrix();
    glRotated(rz, 0, 0, 1);
    glRotated(ry, 0, 1, 0);
    glRotated(rx, 1, 0, 0);
    glScaled(scale.x, scale.y, scale.z);
    child->Draw(t);
    glPopMatrix();
}

void TransformWrapper::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    drawLocal(t);
    glPopMatrix();
}

void TransformWrapper::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double t)
{
    if (!child)
        return;
    std::vector<std::pair<vec<>, double>> local;
    child->getBoundingSpheres(local, t);
    for (const auto& p : local)
        out.push_back({worldPointFromLocal(p.first, scale, rx, ry, rz, pos),
                       worldRadiusFromLocal(p.first, p.second, scale, rx, ry, rz, pos)});
}
