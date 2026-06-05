#pragma once

#include <GL/glut.h>
#include "vector.h"
#include <vector>
#include "templates.h"
#include <algorithm>
#include <cmath>
#include "render_settings.h"

inline double sign(double x) { return (x > 0) ? 1.0 : ((x < 0) ? -1.0 : 0.0); }

inline vec<> rotateZ(const vec<>& v, double angle) {
    double c = cos(angle);
    double s = sin(angle);
    return vec<>(c * v.x - s * v.y, s * v.x + c * v.y, v.z);
}

inline vec<> rotateY(const vec<>& v, double angle) {
    double c = cos(angle);
    double s = sin(angle);
    return vec<>(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

inline vec<> rotateX(const vec<>& v, double angle) {
    double c = cos(angle);
    double s = sin(angle);
    return vec<>(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
}

inline void mergeSpheres(const vec<>& c1, double r1, const vec<>& c2, double r2,
                         vec<>& out_c, double& out_r) {
    double d = (c1 - c2).len();
    if (d + r2 <= r1) {
        out_c = c1;
        out_r = r1;
    } else if (d + r1 <= r2) {
        out_c = c2;
        out_r = r2;
    } else {
        out_r = (d + r1 + r2) * 0.5;
        out_c = c1 + (c2 - c1) * (out_r - r1) / d;
    }
}

class Sphere {
    double R;
    vec<> C;
    static Sphere Instance;
public:
    Sphere(double R = 1, vec<> C = vec<>(0, 0, 0)) : R(R), C(C) {}
    void Draw() {
        glPushMatrix();
        glTranslated(C.x, C.y, C.z);
        glutWireSphere(R, rs::sph_tiny_slc, rs::sph_tiny_stk);
        glPopMatrix();
    }
    void Rotate(double alpha) {
        glRotated(alpha, 0, 1, 0);
    }
};

struct based {
public:
    double alpha_rot = 10;
    GLuint textureID = 0;
    virtual ~based() = default;
    virtual void Draw(double t) {}
    virtual void AddChild(based* p) {}
    virtual void getBoundingSphere(vec<>& center, double& radius, double t) = 0;
    void setTexture(GLuint texID) { textureID = texID; }
};

struct Planet : public based {
    vec<> C, Color, Coord_par;
    double R;
    double alpha_self_rot;
    std::vector<based*> children;
    static GLUquadric* quad;
public:
    double alpha_rot;
    Planet(vec<> C = vec<>(0, 0, 0), double R = 10, double alpha_rot = 0,
           double alpha_self_rot = 15, vec<> Color = vec<>(1, 1, 0), GLuint texID = 0)
        : C(C), R(R), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot), Color(Color) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        center = C;
        radius = R;
        double rot_angle = alpha_rot * t * M_PI / 180.0;
        double self_angle = alpha_self_rot * M_PI / 180.0; // selfRotate uses constant angle
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            child_center = rotateZ(child_center, rot_angle);
            child_center = rotateZ(child_center, self_angle);
            child_center = child_center + C;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        Rotate(alpha_rot, t);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        selfRotate(alpha_self_rot, t);
        glTranslated(C.x, C.y, C.z);
        if (textureID != 0) gluSphere(quad, R, rs::sph_hi_slc, rs::sph_hi_stk);
        else glutSolidSphere(R, rs::sph_hi_slc, rs::sph_hi_stk);
        for (auto child : children) child->Draw(t);
        glTranslated(-C.x, -C.y, -C.z);
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void Rotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 0, 1);
    }
    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha, 0, 0, 10 * t);
    }
};

struct weirdo : public based {
    vec<> C, Color;
    double R1, R2;
    double alpha_rot;
    double alpha_self_rot;
    std::vector<based*> children;
public:
    weirdo(vec<> C = vec<>(0, 0, 0), double R1 = 10, double R2 = 15,
           double alpha_rot = 0, double alpha_self_rot = 15,
           vec<> Color = vec<>(1, 1, 0), GLuint texID = 0)
        : C(C), R1(R1), R2(R2), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot), Color(Color) {
        textureID = texID;
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        center = C;
        radius = R1 + R2;
        double rot_angle_z = alpha_rot * t * M_PI / 180.0;
        double rot_angle_y = alpha_self_rot * t * M_PI / 180.0;
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            child_center = child_center + vec<>(0, 0, child->alpha_rot * sin(t));
            child_center = rotateZ(child_center, rot_angle_z);
            child_center = rotateY(child_center, rot_angle_y);
            child_center = child_center + C;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        Rotate(alpha_rot, t);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        selfRotate(alpha_self_rot, t);
        glTranslated(C.x, C.y, C.z);
        glutSolidTorus(R1, R2, rs::tor_sides, rs::tor_rings);
        for (auto child : children) {
            glTranslated(0, 0, child->alpha_rot * sin(t));
            child->Draw(t);
        }
        glTranslated(-C.x, -C.y, -C.z);
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void Rotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 0, 1);
    }
    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }
};

struct cylinder : public based {
    vec<> C, Color, tmp, Vrac;
    double R, length;
    double alpha_rot, alpha_self_rot;
    std::vector<based*> children;
    double polygons;
    static GLUquadric* quad;
public:
    double rnd_col = 0;
    cylinder(vec<> C = vec<>(0, 0, 0), double polygons = 30, double R = 1,
             double length = 10, double alpha_rot = 0, vec<> Vrac = vec<>(0, 0, 0),
             double alpha_self_rot = 0, vec<> Color = vec<>(1, 1, 1), GLuint texID = 0)
        : C(C), polygons(polygons), R(R), length(length),
          alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot), Vrac(Vrac), Color(Color) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        double actualLength = length * 0.1;
        double halfLength = actualLength * 0.5;
        double maxRad = std::max(R, pow(R, 3.0/2.0));
        radius = sqrt(halfLength * halfLength + maxRad * maxRad);
        vec<> Vrot = rotateZ(Vrac, alpha_rot * t * M_PI / 180.0);
        center = C - Vrot;
        double angle = alpha_rot * t * M_PI / 180.0;
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            child_center = child_center + vec<>(0, 0, child->alpha_rot * sin(t));
            child_center = rotateZ(child_center, angle);
            child_center = child_center - Vrac;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        glTranslated(-Vrac.x, -Vrac.y, -Vrac.z);
        Rotate(alpha_rot, t);
        glPushMatrix();
        glTranslated(C.x - length / 2 * 0.1, C.y, C.z);
        glScaled(0.01, 1, 1);
        if (textureID != 0) gluSphere(quad, R, rs::sph_med_slc, rs::sph_med_stk);
        else glutSolidSphere(R, rs::sph_med_slc, rs::sph_med_stk);
        glPopMatrix();
        glPushMatrix();
        glTranslated(C.x - length / 2 * 0.1, C.y, C.z);
        glBegin(GL_QUAD_STRIP);
        double u_step = 1.0 / polygons;
        for (double i = 0; i <= 2 * 3.14; i += 3.14 * 2 / polygons) {
            double u = i / (2 * 3.14);
            double z1 = R * cos(i);
            double z2 = R * cos(i + 3.14 * 2 / polygons);
            double y1 = R * sin(i);
            double y2 = R * sin(i + 3.14 / 180 * polygons);
            vec<> v = -(vec<>(length, 0, 0) ^ vec<>(0, y2 - y1, z2 - z1));
            glNormal3d(v.x, v.y, v.z);
            if (textureID != 0) glTexCoord2f(u, 0);
            glVertex3d(0, y1, z1);
            if (textureID != 0) glTexCoord2f(u, 1);
            glVertex3d(length * 0.1, y1, z1);
            if (textureID != 0) glTexCoord2f(u + u_step, 0);
            glVertex3d(0, y2, z2);
            if (textureID != 0) glTexCoord2f(u + u_step, 1);
            glVertex3d(length * 0.1, y2, z2);
        }
        glEnd();
        glPopMatrix();
        glPushMatrix();
        glTranslated(C.x + length / 2 * 0.1, C.y, C.z);
        glScaled(0.01, 1, 1);
        if (textureID != 0) gluSphere(quad, pow(R, 3 / 2), rs::sph_med_slc, rs::sph_med_stk);
        else glutSolidSphere(pow(R, 3 / 2), rs::sph_med_slc, rs::sph_med_stk);
        glPopMatrix();
        for (auto child : children) {
            glTranslated(0, 0, child->alpha_rot * sin(t));
            child->Draw(t);
        }
        glTranslated(-C.x, -C.y, -C.z);
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void Rotate(double alpha, double t = 1) {
        glRotated((alpha * t), 0, 0, 1);
    }
    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }
};

struct fucked_cylinder : public based {
    vec<> C, Color, tmp, Vrac;
    double R1, R2, length;
    double alpha_rot, alpha_self_rot;
    std::vector<based*> children;
    double polygons;
    static GLUquadric* quad;
public:
    double rnd_col = 0;
    fucked_cylinder(vec<> C = vec<>(0, 0, 0), double polygons = 30, double R1 = 1,
                    double R2 = 2, double length = 10, double alpha_rot = 0,
                    vec<> Vrac = vec<>(0, 0, 0), double alpha_self_rot = 0,
                    vec<> Color = vec<>(1, 1, 1), GLuint texID = 0)
        : C(C), polygons(polygons), R1(R1), R2(R2), length(length),
          alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot), Vrac(Vrac), Color(Color) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        double maxRad = std::max(std::max(R1, R2), pow(R2, 3.0/2.0));
        radius = length * 0.1 + maxRad;
        vec<> Vrot = rotateZ(Vrac, alpha_rot * t * M_PI / 180.0);
        center = C - Vrot;
        double angle = alpha_rot * t * M_PI / 180.0;
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            child_center = child_center + vec<>(0, 0, child->alpha_rot * sin(t));
            child_center = rotateZ(child_center, angle);
            child_center = child_center - Vrac;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        glTranslated(-Vrac.x, -Vrac.y, -Vrac.z);
        Rotate(alpha_rot, t);
        glPushMatrix();
        glTranslated(C.x - length / 2 * 0.1, C.y, C.z);
        glScaled(0.01, 1, 1);
        if (textureID != 0) gluSphere(quad, R1, rs::sph_med_slc, rs::sph_med_stk);
        else glutSolidSphere(R1, rs::sph_med_slc, rs::sph_med_stk);
        glPopMatrix();
        glPushMatrix();
        glTranslated(C.x - length / 2 * 0.1, C.y, C.z);
        glBegin(GL_QUAD_STRIP);
        double u_step = 1.0 / polygons;
        for (double i = 0; i <= 2 * 3.14; i += 3.14 * 2 / polygons) {
            double u = i / (2 * 3.14);
            double z1 = R1 * cos(i);
            double z1_a = R2 * cos(i);
            double z2 = R1 * cos(i + 3.14 * 2 / polygons);
            double z2_a = R2 * cos(i + 3.14 * 2 / polygons);
            double y1 = R1 * sin(i);
            double y1_a = R2 * sin(i);
            double y2 = R1 * sin(i + 3.14 / 180 * polygons);
            double y2_a = R2 * sin(i + 3.14 / 180 * polygons);
            vec<> v = -(vec<>(length, y1_a - y1, z1_a - z1) ^ vec<>(0, y2 - y1, z2 - z1));
            glNormal3d(v.x, v.y, v.z);
            if (textureID != 0) glTexCoord2f(u, 0);
            glVertex3d(0, y1, z1);
            if (textureID != 0) glTexCoord2f(u, 1);
            glVertex3d(length * 0.1, y1_a, z1_a);
            if (textureID != 0) glTexCoord2f(u + u_step, 0);
            glVertex3d(0, y2, z2);
            if (textureID != 0) glTexCoord2f(u + u_step, 1);
            glVertex3d(length * 0.1, y2_a, z2_a);
        }
        glEnd();
        glPopMatrix();
        glPushMatrix();
        glTranslated(C.x + length / 2 * 0.1, C.y, C.z);
        glScaled(0.01, 1, 1);
        if (textureID != 0) gluSphere(quad, pow(R2, 3 / 2), rs::sph_med_slc, rs::sph_med_stk);
        else glutSolidSphere(pow(R2, 3 / 2), rs::sph_med_slc, rs::sph_med_stk);
        glPopMatrix();
        for (auto child : children) {
            glTranslated(0, 0, child->alpha_rot * sin(t));
            child->Draw(t);
        }
        glTranslated(-C.x, -C.y, -C.z);
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void Rotate(double alpha, double t = 1) {
        glRotated((alpha * t), 0, 0, 1);
    }
    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }
};

struct kabasik : public based {
    vec<> C, Color, Vrac;
    double size;
    double alpha_rot;
    double alpha_self_rot;
    std::vector<based*> children;
    cylinder body, leg1, leg2, leg3, leg4;
    Planet head;
    static GLUquadric* quad;
public:
    kabasik(vec<> C = vec<>().Rnd(vec<>().R(-80, 80), 5, vec<>().R(-80, 80)),
            vec<> Color = vec<>().RndCol(), double size = 1,
            double alpha_rot = vec<>().R(10, 90),
            double alpha_self_rot = vec<>().R(60, 180),
            vec<> Vrac = vec<>().Rnd(vec<>().R(-10, 10), 0, vec<>().R(-10, 10)),
            GLuint texID = 0)
        : C(C), size(size), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot),
          Color(Color), Vrac(Vrac),
          body(vec<>(0, 0, 0), 30, 1, 100, 0, vec<>(0, 0, 0), 0, Color, texID),
          leg1(vec<>(0 - 0.5 * 4, 0 + 40 * 0.1 / 2, 0 - 0.5 * 2), 180, 0.5, 40, 0, vec<>(0, 0, 0), 0, Color, texID),
          leg2(vec<>(0 - 0.5 * 4, 0 + 40 * 0.1 / 2, 0 + 0.5 * 2), 30, 0.5, 40, 0, vec<>(0, 0, 0), 0, Color, texID),
          leg3(vec<>(0 - 0.5 * 4, 0 - 40 * 0.1 / 2, 0 - 0.5 * 2), 30, 0.5, 40, 0, vec<>(0, 0, 0), 0, Color, texID),
          leg4(vec<>(0 - 0.5 * 4, 0 - 40 * 0.1 / 2, 0 + 0.5 * 2), 30, 0.5, 40, 0, vec<>(0, 0, 0), 0, Color, texID),
          head(vec<>(0 - 1 * 5, 0 + 1, 0), 2, 0, 15, vec<>(1, 0, 0), texID) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        // Combined outer rotation (first two rotations from Draw)
        double outer_angle = (90 * sign(Vrac.x) - alpha_rot * t) * M_PI / 180.0;
        // Self-rotation (applied after scaling, before translations)
        double self_angle = alpha_self_rot * t * M_PI / 180.0;
        vec<> trans = C + Vrac;  // combined translation

        // Approximate local bounding sphere of the kabasik geometry
        vec<> local_center(0.0, 0.0, 0.0);
        double local_radius = 8.0;  // encloses all parts

        // Apply transformations in the same order as Draw:
        // 1. Scale
        vec<> scaled_center = local_center * size;
        // 2. Self‑rotation
        vec<> rotated_center = rotateY(scaled_center, self_angle);
        // 3. Translation by C+Vrac
        vec<> translated_center = rotated_center + trans;
        // 4. Outer rotation
        vec<> world_center = rotateY(translated_center, outer_angle);
        double world_radius = local_radius * size;

        center = world_center;
        radius = world_radius;

        // Merge children
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            // Apply child's own offset (Z translation) and then parent transformations
            child_center = child_center + vec<>(0, 0, child->alpha_rot * sin(t));
            child_center = child_center * size;
            child_center = rotateY(child_center, self_angle);
            child_center = child_center + trans;
            child_center = rotateY(child_center, outer_angle);
            child_radius *= size;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        glRotated(90 * sign(Vrac.x), 0, 1, 0);
        glRotated(-alpha_rot * t, 0, 1, 0);
        glTranslated(Vrac.x, Vrac.y, Vrac.z);
        glTranslated(C.x, C.y, C.z);
        selfRotate(alpha_self_rot, t);
        glScaled(size, size, size);
        if (size > 1 && textureID == 0) {
            glColor3d(abs(sin(t)), (int)t % 10 / 10, abs(cos(t)));
        }
        body.Draw(t);
        head.Draw(t);
        glPushMatrix();
        glRotated(90, 0, 0, 1);
        glRotated(sin(10 * t) * 10, 0, 0, 1);
        leg1.Draw(t);
        glPopMatrix();
        glPushMatrix();
        glRotated(90, 0, 0, 1);
        glRotated(-sin(10 * t) * 10, 0, 0, 1);
        leg2.Draw(t);
        glPopMatrix();
        glPushMatrix();
        glRotated(90, 0, 0, 1);
        glRotated(sin(10 * t) * 10, 0, 0, 1);
        leg3.Draw(t);
        glPopMatrix();
        glPushMatrix();
        glRotated(90, 0, 0, 1);
        glRotated(-sin(10 * t) * 10, 0, 0, 1);
        leg4.Draw(t);
        glPopMatrix();
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(0.5, 0.5, 1);
        }
        glRotated(90, 0, 1, 0);
        glTranslated(0, 1. / 8, 5);
        if (textureID != 0) gluCylinder(quad, 1.0 / 1.5, 0, 1 * (sin(t) + 2), rs::cone_seg, rs::cone_seg);
        else glutSolidCone(1.0 / 1.5, 1 * (sin(t) + 2), rs::cone_seg, rs::cone_seg);
        glPopMatrix();
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(0.5, 0.5, 1);
        }
        glRotated(90, 0, 1, 0);
        glTranslated(0, 1. / 8, -8);
        if (textureID != 0) gluCylinder(quad, 1.0 / 1.5 * (cos(t) + 2), 0, 1 * (sin(t) + 2), rs::cone_seg, rs::cone_seg);
        else glutSolidCone(1.0 / 1.5 * (cos(t) + 2), 1 * (sin(t) + 2), rs::cone_seg, rs::cone_seg);
        glPopMatrix();
        for (auto child : children) {
            glTranslated(0, 0, child->alpha_rot * sin(t));
            child->Draw(t);
        }
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }
};

struct tree : public based {
    vec<> C, Color;
    double length;
    double alpha_rot;
    double alpha_self_rot;
    std::vector<based*> children;
    static GLUquadric* quad;
public:
    tree(vec<> C = vec<>().Rnd(vec<>().R(-50, 50), 0, vec<>().R(-50, 50)),
         vec<> Color = vec<>(0, 1, 0), double length = vec<>().R(5, 30),
         double alpha_rot = 0, double alpha_self_rot = 0, GLuint texID = 0)
        : C(C), length(length), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot), Color(Color) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        double H = length / 4.0;
        double R_cone = length / 10.0;
        double step_z = 0.9 * H;
        double rot_y_step = (30.0 * sin(t)) * M_PI / 180.0;

        // Compute number of cones N exactly as in Draw loop
        double loglen = std::log(length);
        int intlog = static_cast<int>(loglen);
        if (intlog == 0) intlog = 1; // avoid division by zero
        double start_d = length / intlog * 3.0;
        int i_start = static_cast<int>(start_d);
        double step_d = length / (intlog * 3.0);
        int N = 0;
        if (step_d > 0) {
            int i = i_start;
            while (i < length * 1.5) {
                N++;
                i = static_cast<int>(i + step_d);
            }
        }

        // Helper to apply initial transformations (RotateZ, RotateY, Translate, RotateX)
        auto applyInitial = [&](const vec<>& p) -> vec<> {
            vec<> v = p;
            v = rotateX(v, -M_PI / 2.0);           // RotateX(-90)
            v = v + C;                             // Translate by C
            v = rotateY(v, alpha_self_rot * t * M_PI / 180.0); // RotateY
            v = rotateZ(v, alpha_rot * t * M_PI / 180.0);      // RotateZ
            return v;
        };

        // Helper to apply k steps of (translate then rotateY)
        auto applySteps = [&](const vec<>& p, int steps) -> vec<> {
            vec<> v = p;
            for (int i = 0; i < steps; ++i) {
                v = v + vec<>(0, 0, step_z);
                v = rotateY(v, rot_y_step);
            }
            return v;
        };

        vec<> local_cone_center(0, 0, H / 2.0);
        double local_cone_radius = sqrt(R_cone * R_cone + (H / 2.0) * (H / 2.0));

        bool first = true;
        vec<> overall_center;
        double overall_radius = 0.0;

        // Merge all cones
        for (int k = 0; k < N; ++k) {
            vec<> p = applySteps(local_cone_center, k);
            vec<> world_center = applyInitial(p);
            if (first) {
                overall_center = world_center;
                overall_radius = local_cone_radius;
                first = false;
            } else {
                mergeSpheres(overall_center, overall_radius, world_center, local_cone_radius, overall_center, overall_radius);
            }
        }

        // Merge children
        for (auto* child : children) {
            vec<> child_center;
            double child_radius;
            child->getBoundingSphere(child_center, child_radius, t);
            vec<> p = applySteps(child_center, N);
            vec<> world_child_center = applyInitial(p);
            if (first) {
                overall_center = world_child_center;
                overall_radius = child_radius;
                first = false;
            } else {
                mergeSpheres(overall_center, overall_radius, world_child_center, child_radius, overall_center, overall_radius);
            }
        }

        // If nothing (should not happen, but just in case)
        if (first) {
            overall_center = applyInitial(vec<>(0, 0, 0));
            overall_radius = 0.0;
        }

        center = overall_center;
        radius = overall_radius;
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        Rotate(alpha_rot, t);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        selfRotate(alpha_self_rot, t);
        glTranslated(C.x, C.y, C.z);
        glRotated(-90, 1, 0, 0);
        for (int i = length / (int)log(length) * 3; i < length * 1.5;
             i += length / ((int)log(length) * 3)) {
            if (textureID != 0) gluCylinder(quad, length / 10, 0, length / 4, rs::cone_seg, rs::cone_seg);
            else glutSolidCone(length / 10, length / 4, rs::cone_seg, rs::cone_seg);
            glTranslated(0, 0, 0.9 * length / 4);
            glRotated(30 * sin(t), 0, 1, 0);
        }
        for (auto child : children) child->Draw(t);
        glTranslated(-C.x, -C.y, -C.z);
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void Rotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 0, 1);
    }
    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }
};

struct snowflake : public based {
    vec<> C, Color;
    double shades;
    double alpha_rot;
    double alpha_self_rot;
    std::vector<based*> children;
    static GLUquadric* quad;
    double fallSpeed;
    double shrinkSpeed;
    double birthTime;
    vec<> startPos;
    double startSize;

    snowflake(vec<> C = vec<>().Rnd(), double shades = 15,
              double alpha_rot = 0, double alpha_self_rot = 0,
              vec<> Color = vec<>().RndCol(),
              GLuint texID = 0)
        : C(C), shades(shades), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot),
          Color(Color), startPos(C), startSize(1.0), birthTime(0.0) {
        textureID = texID;
        fallSpeed = 0.6;
        shrinkSpeed = 0.006;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        vec<> currentPos;
        double currentSize;
        computeState(t, currentPos, currentSize);

        double rz_angle = alpha_rot * t * M_PI / 180.0;
        double ry_angle = alpha_self_rot * t * M_PI / 180.0;
        double r_inner_angle = -M_PI / 2.0;

        center = rotateZ(currentPos, rz_angle);
        center = rotateY(center, ry_angle);
        center = rotateY(center, r_inner_angle);
        radius = currentSize;

        for (auto* child : children) {
            vec<> child_center;
            double child_radius;
            child->getBoundingSphere(child_center, child_radius, t);
            child_center = child_center + vec<>(0, 0, child->alpha_rot * sin(t));
            child_center = rotateZ(child_center, rz_angle);
            child_center = rotateY(child_center, ry_angle);
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        double elapsed = t - birthTime;
        if (elapsed < 0) elapsed = 0;

        double fallTime = startPos.y / fallSpeed;
        double shrinkTime = startSize / shrinkSpeed;
        double totalLifetime = fallTime + shrinkTime;

        if (elapsed >= totalLifetime) {
            startPos = vec<>().Rnd();
            startSize = 1.0;
            birthTime = t;
            elapsed = 0;
            fallTime = startPos.y / fallSpeed;
            shrinkTime = startSize / shrinkSpeed;
            totalLifetime = fallTime + shrinkTime;
        }

        vec<> currentPos;
        double currentSize;

        if (elapsed < fallTime) {
            currentPos.x = startPos.x;
            currentPos.z = startPos.z;
            currentPos.y = startPos.y - fallSpeed * elapsed;
            currentSize = startSize;
        } else {
            double shrinkElapsed = elapsed - fallTime;
            currentPos.x = startPos.x;
            currentPos.z = startPos.z;
            currentPos.y = 0.0;
            currentSize = startSize - shrinkSpeed * shrinkElapsed;
            if (currentSize < 0) currentSize = 0;
        }

        glPushMatrix();
        Rotate(alpha_rot, t);
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        selfRotate(alpha_self_rot, t);
        glPushMatrix();
        glRotated(-90, 0, 1, 0);
        glTranslated(currentPos.x, currentPos.y, currentPos.z);
        const int snowSeg = static_cast<int>(
            std::min(shades, static_cast<double>(rs::snowflake_max_seg)));
        if (textureID != 0) gluSphere(quad, currentSize, snowSeg, snowSeg);
        else glutSolidSphere(currentSize, snowSeg, snowSeg);
        glPopMatrix();
        for (auto child : children) {
            glTranslated(0, 0, child->alpha_rot * sin(t));
            child->Draw(t);
        }
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void Rotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 0, 1);
    }
    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }

private:
    void computeState(double t, vec<>& outPos, double& outSize) const {
        double elapsed = t - birthTime;
        if (elapsed < 0) elapsed = 0;

        double fallTime = startPos.y / fallSpeed;
        double shrinkTime = startSize / shrinkSpeed;
        double totalLifetime = fallTime + shrinkTime;

        if (elapsed >= totalLifetime) {
            elapsed = totalLifetime;
        }

        if (elapsed < fallTime) {
            outPos.x = startPos.x;
            outPos.z = startPos.z;
            outPos.y = startPos.y - fallSpeed * elapsed;
            outSize = startSize;
        } else {
            double shrinkElapsed = elapsed - fallTime;
            outPos.x = startPos.x;
            outPos.z = startPos.z;
            outPos.y = 0.0;
            outSize = startSize - shrinkSpeed * shrinkElapsed;
            if (outSize < 0) outSize = 0;
        }
    }
};

struct kanar : public based {
    vec<> C, Color, Vrac;
    double size;
    double alpha_rot;
    double alpha_self_rot;
    std::vector<based*> children;
    Planet head;
    static GLUquadric* quad;
public:
    kanar(vec<> C = vec<>().Rnd(vec<>().R(-80, 80), vec<>().R(20, 30), vec<>().R(-80, 80)),
          vec<> Color = vec<>().RndCol(), double size = 1,
          double alpha_rot = vec<>().R(10, 90), double alpha_self_rot = vec<>().R(60, 180),
          vec<> Vrac = vec<>().Rnd(vec<>().R(-10, 10), 0, vec<>().R(-10, 10)),
          GLuint texID = 0)
        : C(C), size(size), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot),
          Color(Color), Vrac(Vrac) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        // Combined outer rotation (first two rotations from Draw)
        double outer_angle = (90 * sign(Vrac.x) - alpha_rot * t) * M_PI / 180.0;
        // Self-rotation (applied after scaling, before translations)
        double self_angle = alpha_self_rot * t * M_PI / 180.0;
        vec<> trans = C + Vrac;  // combined translation

        // Approximate local bounding sphere of the kabasik geometry
        vec<> local_center(0.0, 0.0, 0.0);
        double local_radius = 4.07;  // encloses all parts

        // Apply transformations in the same order as Draw:
        // 1. Scale
        vec<> scaled_center = local_center * size;
        // 2. Self‑rotation
        vec<> rotated_center = rotateY(scaled_center, self_angle);
        // 3. Translation by C+Vrac
        vec<> translated_center = rotated_center + trans;
        // 4. Outer rotation
        vec<> world_center = rotateY(translated_center, outer_angle);
        double world_radius = local_radius * size;

        center = world_center;
        radius = world_radius;

        // Merge children
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            // Apply child's own offset (Z translation) and then parent transformations
            child_center = child_center + vec<>(0, 0, child->alpha_rot * sin(t));
            child_center = child_center * size;
            child_center = rotateY(child_center, self_angle);
            child_center = child_center + trans;
            child_center = rotateY(child_center, outer_angle);
            child_radius *= size;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        glRotated(90 * sign(Vrac.x), 0, 1, 0);
        glRotated(-alpha_rot * t, 0, 1, 0);
        glTranslated(Vrac.x, Vrac.y, Vrac.z);
        glTranslated(C.x, C.y, C.z);
        glScaled(size, size, size);
        if (size > 1 && textureID == 0) {
            glColor3d(abs(sin(t)), (int)t % 10 / 10, abs(cos(t)));
        }
        if (textureID != 0) gluSphere(quad, 2, rs::sph_lo_slc, rs::sph_lo_stk);
        else glutSolidSphere(2, rs::sph_lo_slc, rs::sph_lo_stk);
        glColor3d(1, 0, 1);
        if (textureID != 0) glColor4d(1, 1, 1, 1);
        glPushMatrix();
        glRotated(90, 0, 1, 0);
        glTranslated(0, 1.5 / 2, 2);
        glRotated(40 * sin(t), 1, 0, 0);
        glScaled(1, 0.1, 1);
        if (textureID != 0) gluCylinder(quad, 1, 0, 2, rs::cone_seg, rs::cone_seg);
        else glutSolidCone(1, 2, rs::cone_seg, rs::cone_seg);
        glRotated(180, 0, 1, 0);
        glTranslated(0, 0, -4);
        glTranslated(0, 0, 2);
        glRotated(80 * sin(t), 1, 0, 0);
        glTranslated(0, 0, -2);
        glScaled(1, 0.1, 1);
        if (textureID != 0) gluCylinder(quad, 1, 0, 2, rs::cone_seg, rs::cone_seg);
        else glutSolidCone(1, 2, rs::cone_seg, rs::cone_seg);
        glPopMatrix();
        glPushMatrix();
        glRotated(-90, 0, 1, 0);
        glTranslated(0, 1.5 / 2, 2);
        glRotated(40 * sin(t), 1, 0, 0);
        glScaled(1, 0.1, 1);
        if (textureID != 0) gluCylinder(quad, 1, 0, 2, rs::cone_seg, rs::cone_seg);
        else glutSolidCone(1, 2, rs::cone_seg, rs::cone_seg);
        glRotated(-180, 0, 1, 0);
        glTranslated(0, 0, -4);
        glTranslated(0, 0, 2);
        glRotated(80 * sin(t), 1, 0, 0);
        glTranslated(0, 0, -2);
        glScaled(1, 0.1, 1);
        if (textureID != 0) gluCylinder(quad, 1, 0, 2, rs::cone_seg, rs::cone_seg);
        else glutSolidCone(1, 2, rs::cone_seg, rs::cone_seg);
        glPopMatrix();
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }

    void selfRotate(double alpha, double t = 1) {
        glRotated(alpha * t, 0, 1, 0);
    }
};

struct snowman : public based {
    vec<> C, Color, Vrac;
    double size;
    double alpha_rot;
    double alpha_self_rot;
    std::vector<based*> children;
    Planet head;
    static GLUquadric* quad;
public:
    snowman(vec<> C = vec<>().Rnd(vec<>().R(-80, 80), 4, vec<>().R(-80, 80)),
            vec<> Color = vec<>().RndCol(), double size = 1,
            double alpha_rot = vec<>().R(10, 90), double alpha_self_rot = vec<>().R(60, 180),
            vec<> Vrac = vec<>().Rnd(vec<>().R(-10, 10), 0, vec<>().R(-10, 10)),
            GLuint texID = 0)
        : C(C), size(size), alpha_rot(alpha_rot), alpha_self_rot(alpha_self_rot),
          Color(Color), Vrac(Vrac) {
        textureID = texID;
        if (!quad) {
            quad = gluNewQuadric();
            gluQuadricTexture(quad, GL_TRUE);
        }
    }

    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        const double local_radius = 7.75;
        const vec<> local_center(0.0, 3.75, 0.0);
        vec<> scaled_center = local_center * size;
        double scaled_radius = local_radius * size;
        vec<> translated_center = scaled_center + C + Vrac;
        double angle = -alpha_rot * t * M_PI / 180.0;
        vec<> world_center = rotateY(translated_center, angle);
        center = world_center;
        radius = scaled_radius;
        vec<> child_center;
        double child_radius;
        for (auto* child : children) {
            child->getBoundingSphere(child_center, child_radius, t);
            child_center = child_center * size;
            child_center = child_center + C + Vrac;
            child_center = rotateY(child_center, angle);
            child_radius *= size;
            mergeSpheres(center, radius, child_center, child_radius, center, radius);
        }
    }

    void AddChild(based* p) override {
        children.push_back(p);
    }

    void Draw(double t) override {
        glPushMatrix();
        if (textureID != 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glColor4d(1, 1, 1, 1);
        } else {
            glColor3d(Color.x, Color.y, Color.z);
        }
        glRotated(-alpha_rot * t, 0, 1, 0);
        glTranslated(Vrac.x, Vrac.y, Vrac.z);
        glTranslated(C.x, C.y, C.z);
        glScaled(size, size, size);
        if (size > 1 && textureID == 0) {
            glColor3d(abs(sin(t)), (int)t % 10 / 10, abs(cos(t)));
        }
        if (textureID != 0) gluSphere(quad, 4, rs::sph_lo_slc, rs::sph_lo_stk);
        else glutSolidSphere(4, rs::sph_lo_slc, rs::sph_lo_stk);
        glTranslated(0, 4 + 1.5, 0);
        if (textureID != 0) gluSphere(quad, 3, rs::sph_lo_slc, rs::sph_lo_stk);
        else glutSolidSphere(3, rs::sph_lo_slc, rs::sph_lo_stk);
        glTranslated(0, 3 + 1, 0);
        if (textureID != 0) gluSphere(quad, 2, rs::sph_lo_slc, rs::sph_lo_stk);
        else glutSolidSphere(2, rs::sph_lo_slc, rs::sph_lo_stk);
        glColor3d(vec<>().R(0, 1), vec<>().R(0, 1), vec<>().R(0, 1));
        glTranslated(2, 0, 1);
        glutSolidSphere(0.5, rs::sph_tiny_slc, rs::sph_tiny_stk);
        glTranslated(-4, 0, 1);
        glutSolidSphere(0.5, rs::sph_tiny_slc, rs::sph_tiny_stk);
        glPopMatrix();
        if (textureID != 0) glDisable(GL_TEXTURE_2D);
    }
};

class GroundPlane : public based {
private:
    GLuint textureID;
    int edgeLength1 = 200, edgeLength2 = 200;
public:
    GroundPlane(GLuint texID, int edgeLength1 = 200, int edgeLength2 = 200) : textureID(texID) {}
    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        center = vec<>(0, 0, 0);
        radius = sqrt(edgeLength1*edgeLength1 + edgeLength2*edgeLength2);
    }
    void Draw(double /*t*/) override {
        glColor4d(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(-edgeLength1, -0.001, -edgeLength2);
        glTexCoord2f(1, 0); glVertex3f(-edgeLength1, -0.001, edgeLength2);
        glTexCoord2f(1, 1); glVertex3f(edgeLength1, -0.001,  edgeLength2);
        glTexCoord2f(0, 1); glVertex3f(edgeLength1, -0.001,  -edgeLength2);
        glEnd();
    }
};

class SkySphere : public based {
private:
    GLuint textureID;
    double sphereRadius = 1000;
public:
    SkySphere(GLuint texID, unsigned int sphereRadius = 1000) : textureID(texID) {}
    void getBoundingSphere(vec<>& center, double& radius, double t) override {
        center = vec<>(0, 0, 0);
        radius = sphereRadius;
    }
    void Draw(double /*t*/) override {
        glColor4d(1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        GLUquadric* quad = gluNewQuadric();
        gluQuadricTexture(quad, GL_TRUE);
        gluSphere(quad, sphereRadius, rs::sky_slc, rs::sky_stk);
        gluDeleteQuadric(quad);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }
};
