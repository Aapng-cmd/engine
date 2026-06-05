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

void EditorSphere::getBoundingSphere(vec<>& center, double& rad, double /*t*/)
{
    center = pos;
    double m = std::max({scale.x, scale.y, scale.z});
    rad = std::abs(radius) * m;
}

void EditorSphere::Draw(double /*t*/)
{
    glPushMatrix();
    applyTRS(pos.x, pos.y, pos.z, rx, ry, rz);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, 1);
        gluSphere(quad, std::abs(radius), rs::ed_sph_slc, rs::ed_sph_stk);
        glDisable(GL_TEXTURE_2D);
    } else {
        glColor3d(color.x, color.y, color.z);
        glutSolidSphere(std::abs(radius), rs::ed_sph_slc, rs::ed_sph_stk);
    }
    glPopMatrix();
}

EditorBox::EditorBox(vec<> pos, vec<> scale, double rx, double ry, double rz,
                     double dx, double dy, double dz, vec<> color, GLuint tex)
    : pos(pos), scale(scale), rx(rx), ry(ry), rz(rz), dx(dx), dy(dy), dz(dz), color(color)
{
    textureID = tex;
}

void EditorBox::getBoundingSphere(vec<>& center, double& rad, double /*t*/)
{
    center = pos;
    double hx = 0.5 * std::abs(dx) * std::abs(scale.x);
    double hy = 0.5 * std::abs(dy) * std::abs(scale.y);
    double hz = 0.5 * std::abs(dz) * std::abs(scale.z);
    rad = std::sqrt(hx * hx + hy * hy + hz * hz);
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

void EditorBox::Draw(double /*t*/)
{
    glPushMatrix();
    applyTRS(pos.x, pos.y, pos.z, rx, ry, rz);
    glScaled(scale.x * dx, scale.y * dy, scale.z * dz);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, 1);
        drawBoxUnitCubeTextured();
        glDisable(GL_TEXTURE_2D);
    } else {
        glColor3d(color.x, color.y, color.z);
        glutSolidCube(1.0);
    }
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

void EditorCylinder::getBoundingSphere(vec<>& center, double& rad, double /*t*/)
{
    center = pos;
    double hr = std::abs(baseRadius) * std::max(std::abs(scale.x), std::abs(scale.z));
    double hh = 0.5 * std::abs(height) * std::abs(scale.y);
    rad = std::sqrt(hr * hr + hh * hh);
}

void EditorCylinder::Draw(double /*t*/)
{
    glPushMatrix();
    applyTRS(pos.x, pos.y, pos.z, rx, ry, rz);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, 1);
    } else {
        glColor3d(color.x, color.y, color.z);
    }
    glRotated(-90, 1, 0, 0);
    glTranslatef(0, 0, -static_cast<GLfloat>(height) * 0.5f);
    gluCylinder(quad, std::abs(baseRadius), std::abs(baseRadius), std::abs(height), rs::ed_cyl_slc, 1);
    if (textureID != 0)
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

EditorTorus::EditorTorus(vec<> pos, vec<> scale, double rx, double ry, double rz,
                          double innerR, double outerR, vec<> color, GLuint tex)
    : pos(pos), scale(scale), rx(rx), ry(ry), rz(rz), innerR(innerR), outerR(outerR), color(color)
{
    textureID = tex;
}

void EditorTorus::getBoundingSphere(vec<>& center, double& rad, double /*t*/)
{
    center = pos;
    double m = std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
    rad = (std::abs(innerR) + std::abs(outerR)) * m;
}

void EditorTorus::Draw(double /*t*/)
{
    glPushMatrix();
    applyTRS(pos.x, pos.y, pos.z, rx, ry, rz);
    glScaled(scale.x, scale.y, scale.z);
    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glColor4d(1, 1, 1, 1);
    } else {
        glColor3d(color.x, color.y, color.z);
    }
    glutSolidTorus(std::abs(innerR), std::abs(outerR), rs::ed_tor_s, rs::ed_tor_r);
    if (textureID != 0)
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}
