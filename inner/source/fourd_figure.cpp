#include "fourd_figure.h"

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
    if (shapeType == "tesseract")
        fourd::buildTesseract(sizeParam, verts4, edges4);
    else if (shapeType == "hypersphere")
        fourd::buildHypersphereWire(sizeParam, 10, 6, verts4, edges4);
    else if (shapeType == "pyramid4d") {
        fourd::buildTesseract(sizeParam * 0.5, verts4, edges4);
        verts4.push_back({0, 0, 0, sizeParam * 1.5});
        const size_t apex = verts4.size() - 1;
        for (size_t i = 0; i < 8; ++i)
            edges4.push_back({verts4[i], verts4[apex]});
    }
}

void FourDWireFigure::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    const double r = std::abs(sizeParam) * std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
    out.push_back({pos, r * 1.8});
}

void FourDWireFigure::drawProjected(const Camera4DState& cam) const
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    glRotated(rz, 0, 0, 1);
    glRotated(ry, 0, 1, 0);
    glRotated(rx, 1, 0, 0);
    glScaled(scale.x, scale.y, scale.z);
    glColor4d(color.x, color.y, color.z, renderAlpha);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (const Edge4D& e : edges4) {
        vec<> a3, b3;
        if (!fourd::projectTo3D(cam, e.a, a3) || !fourd::projectTo3D(cam, e.b, b3))
            continue;
        glVertex3d(a3.x, a3.y, a3.z);
        glVertex3d(b3.x, b3.y, b3.z);
    }
    glEnd();
    glPopMatrix();
}

void FourDWireFigure::Draw(double /*t*/)
{
    Camera4DState cam;
    fourd::normalizeCamera(cam);
    drawProjected(cam);
}
