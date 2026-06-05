#include "transform_wrapper.h"
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

/** World center: pos + Rz * Ry * Rx * (scale * lc) — matches glTranslated; glRotated rz,ry,rx; glScaled. */
static vec<> worldCenterFromLocal(const vec<>& lc, const vec<>& scale, double rx, double ry, double rz, const vec<>& pos)
{
    vec<> p(lc.x * scale.x, lc.y * scale.y, lc.z * scale.z);
    p = rotX(p, rx);
    p = rotY(p, ry);
    p = rotZ(p, rz);
    return p + pos;
}

TransformWrapper::TransformWrapper(based* owned, vec<> pos, vec<> scale, double rx, double ry, double rz)
    : child(owned), pos(pos), scale(scale), rx(rx), ry(ry), rz(rz)
{
    textureID = 0;
}

void TransformWrapper::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    glRotated(rz, 0, 0, 1);
    glRotated(ry, 0, 1, 0);
    glRotated(rx, 1, 0, 0);
    glScaled(scale.x, scale.y, scale.z);
    child->Draw(t);
    glPopMatrix();
}

void TransformWrapper::getBoundingSphere(vec<>& center, double& radius, double t)
{
    vec<> lc;
    double lr = 0;
    child->getBoundingSphere(lc, lr, t);
    center = worldCenterFromLocal(lc, scale, rx, ry, rz, pos);
    double mx = std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
    radius = lr * mx * 1.75;
}
