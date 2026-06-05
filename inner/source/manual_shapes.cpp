#include "manual_shapes.h"
#include "render_settings.h"
#include <algorithm>
#include <cmath>

GLUquadric* EditorSphere::quad = nullptr;
GLUquadric* EditorCylinder::quad = nullptr;

static void applyTRS(double px, double py, double pz, double rx, double ry, double rz)
{
    glTranslated(px, py, pz);
    glRotated(rz, 0, 0, 1);
    glRotated(ry, 0, 1, 0);
    glRotated(rx, 1, 0, 0);
}

static void applyRot(double rx, double ry, double rz)
{
    glRotated(rz, 0, 0, 1);
    glRotated(ry, 0, 1, 0);
    glRotated(rx, 1, 0, 0);
}

EditorSphere::EditorSphere(vec<> pos, vec<> scale, double rx, double ry, double rz,
                           double radius, vec<> color, GLuint tex)
    : pos(pos), scale(scale), rx(rx), ry(ry), rz(rz), radius(radius), color(color)
{
    textureID = tex;
    if (!quad) {
        quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluQuadricNormals(quad, GLU_SMOOTH);
    }
}

void EditorSphere::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    const double m = std::max({scale.x, scale.y, scale.z});
    out.push_back({pos, std::abs(radius) * m});
}

void EditorSphere::drawLocal(double /*t*/)
{
    glPushMatrix();
    applyRot(rx, ry, rz);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, renderAlpha);
        gluSphere(quad, std::abs(radius), rs::ed_sph_slc, rs::ed_sph_stk);
        glDisable(GL_TEXTURE_2D);
    } else {
        glColor4d(color.x, color.y, color.z, renderAlpha);
        glutSolidSphere(std::abs(radius), rs::ed_sph_slc, rs::ed_sph_stk);
    }
    glPopMatrix();
}

void EditorSphere::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    drawLocal(t);
    glPopMatrix();
}

EditorBox::EditorBox(vec<> pos, vec<> scale, double rx, double ry, double rz,
                     double dx, double dy, double dz, vec<> color, GLuint tex)
    : pos(pos), scale(scale), rx(rx), ry(ry), rz(rz), dx(dx), dy(dy), dz(dz), color(color)
{
    textureID = tex;
}

void EditorBox::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    const double hx = 0.5 * std::abs(dx) * std::abs(scale.x);
    const double hy = 0.5 * std::abs(dy) * std::abs(scale.y);
    const double hz = 0.5 * std::abs(dz) * std::abs(scale.z);
    out.push_back({pos, std::sqrt(hx * hx + hy * hy + hz * hz)});
}

static void drawBoxUnitCubeTextured()
{
    const float h = 0.5f;
    glBegin(GL_QUADS);
    // +Z
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-h, -h, h);
    glTexCoord2f(1, 0); glVertex3f(h, -h, h);
    glTexCoord2f(1, 1); glVertex3f(h, h, h);
    glTexCoord2f(0, 1); glVertex3f(-h, h, h);
    // -Z
    glNormal3f(0, 0, -1);
    glTexCoord2f(1, 0); glVertex3f(-h, -h, -h);
    glTexCoord2f(0, 0); glVertex3f(h, -h, -h);
    glTexCoord2f(0, 1); glVertex3f(h, h, -h);
    glTexCoord2f(1, 1); glVertex3f(-h, h, -h);
    // +Y
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-h, h, -h);
    glTexCoord2f(1, 0); glVertex3f(h, h, -h);
    glTexCoord2f(1, 1); glVertex3f(h, h, h);
    glTexCoord2f(0, 1); glVertex3f(-h, h, h);
    // -Y
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 1); glVertex3f(-h, -h, -h);
    glTexCoord2f(0, 0); glVertex3f(h, -h, -h);
    glTexCoord2f(1, 0); glVertex3f(h, -h, h);
    glTexCoord2f(1, 1); glVertex3f(-h, -h, h);
    // +X
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(h, -h, -h);
    glTexCoord2f(1, 0); glVertex3f(h, -h, h);
    glTexCoord2f(1, 1); glVertex3f(h, h, h);
    glTexCoord2f(0, 1); glVertex3f(h, h, -h);
    // -X
    glNormal3f(-1, 0, 0);
    glTexCoord2f(1, 0); glVertex3f(-h, -h, -h);
    glTexCoord2f(0, 0); glVertex3f(-h, -h, h);
    glTexCoord2f(0, 1); glVertex3f(-h, h, h);
    glTexCoord2f(1, 1); glVertex3f(-h, h, -h);
    glEnd();
}

void EditorBox::drawLocal(double /*t*/)
{
    glPushMatrix();
    applyRot(rx, ry, rz);
    glScaled(scale.x * dx, scale.y * dy, scale.z * dz);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, renderAlpha);
        drawBoxUnitCubeTextured();
        glDisable(GL_TEXTURE_2D);
    } else {
        glColor4d(color.x, color.y, color.z, renderAlpha);
        glutSolidCube(1.0);
    }
    glPopMatrix();
}

void EditorBox::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    drawLocal(t);
    glPopMatrix();
}

EditorCylinder::EditorCylinder(vec<> pos, vec<> scale, double rx, double ry, double rz,
                               double baseRadius, double height, vec<> color, GLuint tex)
    : pos(pos), scale(scale), rx(rx), ry(ry), rz(rz), baseRadius(baseRadius), height(height), color(color)
{
    textureID = tex;
    if (!quad) {
        quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluQuadricNormals(quad, GLU_SMOOTH);
    }
}

void EditorCylinder::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    const double hr = std::abs(baseRadius) * std::max(std::abs(scale.x), std::abs(scale.z));
    const double hh = 0.5 * std::abs(height) * std::abs(scale.y);
    out.push_back({pos, std::sqrt(hr * hr + hh * hh)});
}

void EditorCylinder::drawLocal(double /*t*/)
{
    glPushMatrix();
    applyRot(rx, ry, rz);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, renderAlpha);
    } else {
        glColor4d(color.x, color.y, color.z, renderAlpha);
    }
    glRotated(-90, 1, 0, 0);
    glTranslatef(0, 0, -static_cast<GLfloat>(height) * 0.5f);
    gluCylinder(quad, std::abs(baseRadius), std::abs(baseRadius), std::abs(height), rs::ed_cyl_slc, 1);
    if (textureID != 0)
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void EditorCylinder::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    drawLocal(t);
    glPopMatrix();
}

EditorTorus::EditorTorus(vec<> pos, vec<> scale, double rx, double ry, double rz,
                          double innerR, double outerR, vec<> color, GLuint tex)
    : pos(pos), scale(scale), rx(rx), ry(ry), rz(rz), innerR(innerR), outerR(outerR), color(color)
{
    textureID = tex;
}

void EditorTorus::getBoundingSpheres(std::vector<std::pair<vec<>, double>>& out, double /*t*/)
{
    const double m = std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
    out.push_back({pos, (std::abs(innerR) + std::abs(outerR)) * m});
}

void EditorTorus::drawLocal(double /*t*/)
{
    glPushMatrix();
    applyRot(rx, ry, rz);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, renderAlpha);
    } else {
        glColor4d(color.x, color.y, color.z, renderAlpha);
    }
    glutSolidTorus(std::abs(innerR), std::abs(outerR), rs::ed_tor_s, rs::ed_tor_r);
    if (textureID != 0)
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void EditorTorus::Draw(double t)
{
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    drawLocal(t);
    glPopMatrix();
}
