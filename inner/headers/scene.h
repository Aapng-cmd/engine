#pragma once

#include "collision_mesh.h"
#include "fourd_collision.h"
#include "fourd_figure.h"
#include "fourd_math.h"
#include "collision_repr.h"
#include "figures.h"
#include "manual_shapes.h"
#include "object_factory.h"
#include "transform_wrapper.h"
#include "vector.h"
#include <cmath>
#include "templates.h"
#include "textures.h"
#include "textures_path.h"
#include <functional>
#include <algorithm>
#include <vector>

/* TEXTURES_PATH() / texturesPath() — absolute shared folder .../driver_test/textures
   (inner dir + "/../textures"). LoadTexID("textures/foo.png") resolves via that. */

struct Scene
{
    enum class ShapeKind { Sphere, Box, Cylinder, Torus, Compound, Other };
    struct PartSphereLocal {
        vec<> offset;
        double radius = 1;
    };

    struct BodyState {
        based* obj = nullptr;
        ShapeKind kind = ShapeKind::Other;
        vec<> baseCenter;
        /** Scene placement (TransformWrapper pos or editor pos). */
        vec<> anchor;
        /** baseCenter - anchor at init; offset from anchor to COM in body frame. */
        vec<> comOffset;
        vec<> center;
        std::vector<PartSphereLocal> partsLocal;
        /** Triangle collision parts relative to baseCenter (body frame). */
        std::vector<CollTri> partsTriLocal;
        CollisionRepr collisionRepr = CollisionRepr::Sphere;
        vec<> velocity;
        vec<> angularVelocity;
        vec<> spinAxis = vec<>(0, 1, 0);
        double spinDeg = 0;
        double radius = 1;
        double mass = 1;
        double invMass = 1;
        double inertia = 1;
        double invInertia = 1;
        vec<> halfExtents;
        double cylRadius = 1;
        double cylHalfHeight = 0.5;
        double torMajor = 1;
        double torMinor = 0.3;
        int groupId = -1;
        int useGravity = 0;
        int useFriction = 0;
        int collide = 1;
        double alpha = 1.0;
        vec<> gravity = vec<>(0, -9.81, 0);
        double groundFriction = 0.0;
        double restitution = 0.12;
        int leader = -1;
        bool isLeader = true;
        vec<> localFromCom;
        /** K-слой: 3D-тела фиксированы на kPos; 4D — динамика по K. */
        double kPos = 0.0;
        double kVel = 0.0;
        bool is4D = false;
        double hyperRadius = 1.0;
    };

    static constexpr double kSliceHalf = 0.25;

    struct Contact {
        vec<> point;
        vec<> normal;
        double penetration = 0;
    };
    struct ObjectPhysics {
        vec<> velocity;
        vec<> orbitCenter;
        double orbitOmegaY = 0;
        int groupId = -1;
        int useGravity = 0;
        int useFriction = 0;
        int collide = 1;
        double alpha = 1.0;
        /** Total mass; <= 0 means auto from part geometry (volume-weighted COM). */
        double massOverride = 0.0;
        vec<> gravity = vec<>(0, -9.81, 0);
        double groundFriction = 0.0;
        double restitution = 0.12;
        double pk = 0.0;
        double vk = 0.0;
    };

    std::vector<based*> Objects;
    std::vector<ObjectPhysics> objectPhysics;
    std::vector<based*> texturedObjects;
    based* envGround = nullptr;
    based* envSky = nullptr;
    float sceneLightPos[4] = {40.f, 120.f, 60.f, 1.f};
    std::vector<std::function<based*()>> objectList;
    std::vector<std::string> texturePaths;
    std::vector<BodyState> bodies;
    std::vector<vec<>> groupCom;
    double physicsTime = 0.0;
    bool physicsInitialized = false;
    /** Множитель шага физики (0.2…3.0), клавиши +/− в scene_viewer. */
    double physicsTimeScale = 1.0;
    /** Позиция камеры для LOD коллизий (--O1). */
    vec<> physicsCameraPos = vec<>(0, 25, 45);

public:
    unsigned int lastDrawnCount = 0;
    /** 0=выкл, 1=границы (';'), 2=COM/скорость/траектория (отдельно от слоя 1). */
    int debugLayer = 0;
    bool use4dCamera = false;
    Camera4DState camera4d;
    double camera4dK = -2.0;
    bool showHud = true;
    void clearEnvironment()
    {
        delete envGround;
        envGround = nullptr;
        delete envSky;
        envSky = nullptr;
        for (based* p : texturedObjects)
            delete p;
        texturedObjects.clear();
    }

    void setEnvironment(const std::string& groundTex, int groundE1, int groundE2, const std::string& skyTex,
                        unsigned skyRadius)
    {
        clearEnvironment();
        if (!skyTex.empty())
            envSky = new SkySphere(LoadTexID(skyTex), skyRadius);
        if (!groundTex.empty()) {
            auto* gp = new GroundPlane(LoadTexID(groundTex), groundE1, groundE2);
            const bool water = groundTex.find("water") != std::string::npos;
            gp->setReflect(water ? 1.0 : 0.0, water);
            envGround = gp;
        }
    }

    void updateSceneLight(const vec<>& camPos)
    {
        sceneLightPos[0] = static_cast<float>(camPos.x + 50.0);
        sceneLightPos[1] = static_cast<float>(camPos.y + 80.0);
        sceneLightPos[2] = static_cast<float>(camPos.z + 30.0);
        glLightfv(GL_LIGHT0, GL_POSITION, sceneLightPos);
    }

    Scene(void) {}
    
    void addObject(int i) {
        if (objectList.empty()) return;
        based* k = objectList[i % objectList.size()]();
        k->setTexture(LoadTexID(texturePaths[i % objectList.size()]));
        Objects.push_back(k);
        objectPhysics.push_back(ObjectPhysics());
        physicsInitialized = false;
    }

    void addLoadedObject(based* o, const ObjectPhysics& p) {
        Objects.push_back(o);
        objectPhysics.push_back(p);
        physicsInitialized = false;
    }

    void Draw(void) {}
    
    vec<> transformPoint(const vec<>& p, const double mat[16]) {
        return vec<>(
            mat[0]*p.x + mat[4]*p.y + mat[8]*p.z + mat[12],
            mat[1]*p.x + mat[5]*p.y + mat[9]*p.z + mat[13],
            mat[2]*p.x + mat[6]*p.y + mat[10]*p.z + mat[14]
        );
    }
    
    // очень интересно, надо разобраться TODO
    // оказывается - решение уравнений для понимания граней усечённой пирамиды (то, как видит камера)
    void extractFrustumPlanesFromProj(double planes[6][4], const double proj[16]) {
        // Extract rows of the projection matrix (column‑major storage)
        double row0[4] = { proj[0], proj[4], proj[8], proj[12] };
        double row1[4] = { proj[1], proj[5], proj[9], proj[13] };
        double row2[4] = { proj[2], proj[6], proj[10], proj[14] };
        double row3[4] = { proj[3], proj[7], proj[11], proj[15] };

        // Right plane: row3 - row0
        planes[0][0] = row3[0] - row0[0];
        planes[0][1] = row3[1] - row0[1];
        planes[0][2] = row3[2] - row0[2];
        planes[0][3] = row3[3] - row0[3];
        // Left plane: row3 + row0
        planes[1][0] = row3[0] + row0[0];
        planes[1][1] = row3[1] + row0[1];
        planes[1][2] = row3[2] + row0[2];
        planes[1][3] = row3[3] + row0[3];
        // Bottom plane: row3 + row1
        planes[2][0] = row3[0] + row1[0];
        planes[2][1] = row3[1] + row1[1];
        planes[2][2] = row3[2] + row1[2];
        planes[2][3] = row3[3] + row1[3];
        // Top plane: row3 - row1
        planes[3][0] = row3[0] - row1[0];
        planes[3][1] = row3[1] - row1[1];
        planes[3][2] = row3[2] - row1[2];
        planes[3][3] = row3[3] - row1[3];
        // Near plane: row3 + row2
        planes[4][0] = row3[0] + row2[0];
        planes[4][1] = row3[1] + row2[1];
        planes[4][2] = row3[2] + row2[2];
        planes[4][3] = row3[3] + row2[3];
        // Far plane: row3 - row2
        planes[5][0] = row3[0] - row2[0];
        planes[5][1] = row3[1] - row2[1];
        planes[5][2] = row3[2] - row2[2];
        planes[5][3] = row3[3] - row2[3];

        // Normalize the plane equations
        for (int i = 0; i < 6; i++) {
            double len = sqrt(planes[i][0]*planes[i][0] + planes[i][1]*planes[i][1] + planes[i][2]*planes[i][2]);
            if (len != 0.0) {
                planes[i][0] /= len;
                planes[i][1] /= len;
                planes[i][2] /= len;
                planes[i][3] /= len;
            }
        }
    }
    
    void multiplyMatrices(const double a[16], const double b[16], double out[16]) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                out[j*4 + i] = 0.0;
                for (int k = 0; k < 4; ++k) {
                    out[j*4 + i] += a[k*4 + i] * b[j*4 + k];
                }
            }
        }
    }

    // а здесь смотрим, видит ли камера описывающую сферу
    // если видит, то отрисовываем
    bool sphereInFrustum(double planes[6][4], const vec<>& center, double radius) {
        for (int i = 0; i < 6; i++) {
            double dist = planes[i][0]*center.x + planes[i][1]*center.y + planes[i][2]*center.z + planes[i][3];
            if (dist + radius < -1e-6) return false;
        }
        return true;
    }

    static double safeLen(const vec<>& v) { return std::sqrt(std::max(0.0, v.len2())); }
    static vec<> safeNorm(const vec<>& v, const vec<>& fallback = vec<>(0, 1, 0)) {
        double l2 = v.len2();
        if (l2 < 1e-18) return fallback;
        return v * (1.0 / std::sqrt(l2));
    }
    static vec<> rotateAroundAxis(const vec<>& v, const vec<>& axis, double angleRad) {
        vec<> a = safeNorm(axis, vec<>(0, 1, 0));
        double c = std::cos(angleRad), s = std::sin(angleRad);
        return v * c + (a ^ v) * s + a * (a.dot(v) * (1.0 - c));
    }

    /** Multi-part sphere decomposition: translate only. Single mesh / primitives may spin. */
    static bool bodyUsesRotation(const BodyState& b)
    {
        if (b.kind == ShapeKind::Compound || b.kind == ShapeKind::Other)
            return false;
        if (!b.partsTriLocal.empty())
            return true;
        return b.partsLocal.size() <= 1;
    }

    static bool bodyUsesTriangleCollision(const BodyState& b)
    {
        return b.collisionRepr == CollisionRepr::Triangle && !b.partsTriLocal.empty();
    }

    static vec<> getObjectAnchor(based* o)
    {
        if (auto* w = dynamic_cast<TransformWrapper*>(o))
            return w->getPos();
        if (auto* s = dynamic_cast<EditorSphere*>(o))
            return s->pos;
        if (auto* bx = dynamic_cast<EditorBox*>(o))
            return bx->pos;
        if (auto* cy = dynamic_cast<EditorCylinder*>(o))
            return cy->pos;
        if (auto* to = dynamic_cast<EditorTorus*>(o))
            return to->pos;
        vec<> c;
        double r = 0;
        o->emergency_bounding_sphere_calc_protocol(c, r, 0);
        return c;
    }

    static vec<> figureColor(based* el)
    {
        if (auto* bx = dynamic_cast<EditorBox*>(el))
            return bx->color;
        if (auto* s = dynamic_cast<EditorSphere*>(el))
            return s->color;
        if (auto* cy = dynamic_cast<EditorCylinder*>(el))
            return cy->color;
        if (auto* to = dynamic_cast<EditorTorus*>(el))
            return to->color;
        if (auto* f4 = dynamic_cast<FourDWireFigure*>(el))
            return f4->color;
        return vec<>(0.75, 0.75, 0.75);
    }

    static void drawMeshBodyVisual(const BodyState& b, based* el, double t)
    {
        std::vector<CollTri> wt;
        worldPartTriangles(b, t, wt);
        if (wt.empty())
            return;
        const vec<> col = figureColor(el);
        glColor4d(col.x, col.y, col.z, el->renderAlpha);
        glEnable(GL_NORMALIZE);
        for (const CollTri& tri : wt) {
            const vec<> n = tri.normal();
            glBegin(GL_TRIANGLES);
            glNormal3d(n.x, n.y, n.z);
            glVertex3d(tri.v0.x, tri.v0.y, tri.v0.z);
            glVertex3d(tri.v1.x, tri.v1.y, tri.v1.z);
            glVertex3d(tri.v2.x, tri.v2.y, tri.v2.z);
            glEnd();
        }
    }

    static void drawObjectRigidBody(based* el, const BodyState& b, double t)
    {
        if (bodyUsesTriangleCollision(b)) {
            drawMeshBodyVisual(b, el, t);
            return;
        }
        glPushMatrix();
        glTranslated(b.center.x, b.center.y, b.center.z);
        if (bodyUsesRotation(b))
            glRotated(b.spinDeg, b.spinAxis.x, b.spinAxis.y, b.spinAxis.z);
        vec<> rel = b.anchor - b.baseCenter;
        glTranslated(rel.x, rel.y, rel.z);
        if (auto* w = dynamic_cast<TransformWrapper*>(el))
            w->drawLocal(t);
        else if (auto* s = dynamic_cast<EditorSphere*>(el))
            s->drawLocal(t);
        else if (auto* bx = dynamic_cast<EditorBox*>(el))
            bx->drawLocal(t);
        else if (auto* cy = dynamic_cast<EditorCylinder*>(el))
            cy->drawLocal(t);
        else if (auto* to = dynamic_cast<EditorTorus*>(el))
            to->drawLocal(t);
        else
            el->Draw(t);
        glPopMatrix();
    }

    static bool bodiesShareKSlice(const BodyState& a, const BodyState& b)
    {
        if (a.is4D || b.is4D)
            return std::abs(a.kPos - b.kPos) <= (a.hyperRadius + b.hyperRadius + kSliceHalf);
        return std::abs(a.kPos - b.kPos) <= kSliceHalf;
    }

    static ShapeKind classifyShape(const based* obj) {
        if (dynamic_cast<const EditorSphere*>(obj))
            return ShapeKind::Sphere;
        if (dynamic_cast<const EditorBox*>(obj))
            return ShapeKind::Box;
        if (dynamic_cast<const EditorCylinder*>(obj))
            return ShapeKind::Cylinder;
        if (dynamic_cast<const EditorTorus*>(obj))
            return ShapeKind::Torus;
        if (dynamic_cast<const TransformWrapper*>(obj)) {
            if (usesTriangleCollision(collisionReprForObject(obj)))
                return ShapeKind::Box;
            return ShapeKind::Compound;
        }
        if (usesTriangleCollision(collisionReprForObject(obj)))
            return ShapeKind::Box;
        return ShapeKind::Other;
    }

    /** Mass of one collision part (uniform density 1): sphere volume at geometric center. */
    static double partMassFromSphere(double radius)
    {
        return std::max(1e-6, (4.0 / 3.0) * M_PI * radius * radius * radius);
    }

    static void initBodyFromPartSpheres(BodyState& b, based* o, double massOverride = 0.0)
    {
        std::vector<std::pair<vec<>, double>> parts;
        o->getBoundingSpheres(parts, 0);
        if (parts.empty()) {
            vec<> c;
            double r = 0;
            o->emergency_bounding_sphere_calc_protocol(c, r, 0);
            parts.push_back({c, r});
        }
        double totalMass = 0.0;
        vec<> com(0, 0, 0);
        for (const auto& p : parts) {
            const double m = partMassFromSphere(p.second);
            com += p.first * m;
            totalMass += m;
        }
        if (totalMass > 1e-9)
            com = com / totalMass;
        else if (!parts.empty())
            com = parts[0].first;

        b.baseCenter = com;
        b.center = com;
        b.partsLocal.clear();
        b.radius = 0.1;
        double inertia = 0.0;
        for (const auto& p : parts) {
            const vec<> off = p.first - com;
            const double m = partMassFromSphere(p.second);
            b.partsLocal.push_back({off, p.second});
            b.radius = std::max(b.radius, off.len() + p.second);
            inertia += 0.4 * m * p.second * p.second + m * off.len2();
        }
        if (massOverride > 1e-9) {
            b.mass = massOverride;
            b.inertia = std::max(1e-6, 0.4 * b.mass * b.radius * b.radius);
        } else {
            b.mass = std::max(0.5, totalMass);
            b.inertia = std::max(1e-6, inertia);
        }
        b.invMass = b.mass > 1e-9 ? 1.0 / b.mass : 0.0;
        b.invInertia = b.inertia > 1e-9 ? 1.0 / b.inertia : 0.0;
    }

    static void initBodyFromTriMesh(BodyState& b, based* o, double massOverride = 0.0)
    {
        std::vector<CollTri> tris;
        const int subdiv = collision::maxSubdivForFaceSize(1.0);
        if (!collision::buildObjectCollisionMesh(o, tris, subdiv) || tris.empty()) {
            initBodyFromPartSpheres(b, o, massOverride);
            return;
        }

        double totalMass = 0.0;
        vec<> com(0, 0, 0);
        for (const CollTri& tri : tris) {
            const double m = std::max(1e-9, tri.area());
            com += tri.centroid() * m;
            totalMass += m;
        }
        if (totalMass > 1e-9)
            com = com / totalMass;
        else
            com = tris[0].centroid();

        b.baseCenter = com;
        b.center = com;
        b.partsLocal.clear();
        b.partsTriLocal.clear();
        b.radius = 0.1;
        vec<> bbMin(1e30, 1e30, 1e30);
        vec<> bbMax(-1e30, -1e30, -1e30);
        double inertia = 0.0;
        for (const CollTri& tri : tris) {
            for (const vec<>* vp : {&tri.v0, &tri.v1, &tri.v2}) {
                bbMin.x = std::min(bbMin.x, vp->x);
                bbMin.y = std::min(bbMin.y, vp->y);
                bbMin.z = std::min(bbMin.z, vp->z);
                bbMax.x = std::max(bbMax.x, vp->x);
                bbMax.y = std::max(bbMax.y, vp->y);
                bbMax.z = std::max(bbMax.z, vp->z);
            }
            CollTri local;
            local.v0 = tri.v0 - com;
            local.v1 = tri.v1 - com;
            local.v2 = tri.v2 - com;
            b.partsTriLocal.push_back(local);
            b.radius = std::max({b.radius, local.v0.len(), local.v1.len(), local.v2.len()});
            const vec<> c = local.centroid();
            const double m = std::max(1e-9, tri.area());
            inertia += m * c.len2();
        }
        b.halfExtents = (bbMax - bbMin) * 0.5;

        if (b.kind == ShapeKind::Box) {
            const double volMass = estimateMass(b);
            b.mass = massOverride > 1e-9 ? massOverride : std::max(0.5, volMass);
            b.inertia = std::max(1e-6, (1.0 / 6.0) * b.mass * b.radius * b.radius * 4.0);
        } else if (massOverride > 1e-9) {
            b.mass = massOverride;
            b.inertia = std::max(1e-6, 0.4 * b.mass * b.radius * b.radius);
        } else {
            b.mass = std::max(0.5, totalMass);
            b.inertia = std::max(1e-6, inertia);
        }
        b.invMass = b.mass > 1e-9 ? 1.0 / b.mass : 0.0;
        b.invInertia = b.inertia > 1e-9 ? 1.0 / b.inertia : 0.0;
        b.collisionRepr = CollisionRepr::Triangle;
    }

    static void worldPartSpheres(const BodyState& b, double tAnim,
                               std::vector<std::pair<vec<>, double>>& out) {
        out.clear();
        const double spinRad = bodyUsesRotation(b) ? b.spinDeg * M_PI / 180.0 : 0.0;
        if (b.obj) {
            std::vector<std::pair<vec<>, double>> rest;
            b.obj->getBoundingSpheres(rest, tAnim);
            for (const auto& s : rest) {
                vec<> rel = s.first - b.baseCenter;
                if (spinRad != 0.0)
                    rel = rotateAroundAxis(rel, b.spinAxis, spinRad);
                out.push_back({b.center + rel, s.second});
            }
        }
        if (out.empty()) {
            for (const auto& pl : b.partsLocal) {
                vec<> off = pl.offset;
                if (spinRad != 0.0)
                    off = rotateAroundAxis(off, b.spinAxis, spinRad);
                out.push_back({b.center + off, pl.radius});
            }
        }
        if (out.empty())
            out.push_back({b.center, b.radius});
    }

    static double meshFaceSizeForBody(const BodyState& b)
    {
        if (b.halfExtents.x > 1e-9 || b.halfExtents.y > 1e-9 || b.halfExtents.z > 1e-9)
            return 2.0 * std::min({std::max(0.25, b.halfExtents.x), std::max(0.25, b.halfExtents.y),
                                   std::max(0.25, b.halfExtents.z)});
        return 1.0;
    }

    int collisionSubdivForBody(const BodyState& b) const
    {
        const double faceSize = meshFaceSizeForBody(b);
        const double dist = (b.center - physicsCameraPos).len();
        return collision::lodFaceSubdiv(faceSize, dist);
    }

    /** Physics / accurate bounds: fixed mesh from init (partsTriLocal). */
    static void worldPartTriangles(const BodyState& b, double /*tAnim*/, std::vector<CollTri>& out)
    {
        out.clear();
        if (b.partsTriLocal.empty())
            return;
        const double spinRad = bodyUsesRotation(b) ? b.spinDeg * M_PI / 180.0 : 0.0;
        for (const CollTri& lt : b.partsTriLocal) {
            auto map = [&](const vec<>& relIn) {
                vec<> rel = relIn;
                if (spinRad != 0.0)
                    rel = rotateAroundAxis(rel, b.spinAxis, spinRad);
                return b.center + rel;
            };
            out.push_back({map(lt.v0), map(lt.v1), map(lt.v2)});
        }
    }

    /** Debug / --O1: rebuild mesh at requested subdiv from camera distance. */
    void worldPartTrianglesLod(const BodyState& b, int faceSubdiv, std::vector<CollTri>& out) const
    {
        out.clear();
        if (!b.obj)
            return;
        const double spin = bodyUsesRotation(b) ? b.spinDeg : 0.0;
        collision::buildWorldCollisionMesh(b.obj, b.center, b.baseCenter, b.spinAxis, spin, faceSubdiv, out);
    }

    static double bodyLowestY(const BodyState& b, double tAnim)
    {
        if (!b.partsTriLocal.empty()) {
            std::vector<CollTri> wt;
            worldPartTriangles(b, tAnim, wt);
            double minY = b.center.y;
            for (const CollTri& tri : wt) {
                minY = std::min({minY, tri.v0.y, tri.v1.y, tri.v2.y});
            }
            return minY;
        }
        return b.center.y - b.radius;
    }

    static double estimateMass(const BodyState& b) {
        if (b.kind == ShapeKind::Box) {
            double dx = 2.0 * b.halfExtents.x, dy = 2.0 * b.halfExtents.y, dz = 2.0 * b.halfExtents.z;
            return std::max(0.5, dx * dy * dz);
        }
        if (b.kind == ShapeKind::Cylinder)
            return std::max(0.5, M_PI * b.cylRadius * b.cylRadius * (2.0 * b.cylHalfHeight));
        if (b.kind == ShapeKind::Torus)
            return std::max(0.5, 2.0 * M_PI * M_PI * b.torMajor * b.torMinor * b.torMinor);
        return std::max(0.5, (4.0 / 3.0) * M_PI * b.radius * b.radius * b.radius);
    }

    void rebuildBodies() {
        std::vector<vec<>> prevCenter, prevVel;
        std::vector<double> prevK, prevKV;
        std::vector<based*> prevObj;
        if (physicsInitialized && bodies.size() == Objects.size()) {
            prevCenter.reserve(bodies.size());
            prevVel.reserve(bodies.size());
            prevK.reserve(bodies.size());
            prevKV.reserve(bodies.size());
            prevObj.reserve(bodies.size());
            for (const BodyState& pb : bodies) {
                prevCenter.push_back(pb.center);
                prevVel.push_back(pb.velocity);
                prevK.push_back(pb.kPos);
                prevKV.push_back(pb.kVel);
                prevObj.push_back(pb.obj);
            }
        }
        bodies.clear();
        bodies.reserve(Objects.size());
        groupCom.clear();
        for (size_t i = 0; i < Objects.size(); ++i) {
            based* o = Objects[i];
            BodyState b;
            b.obj = o;
            b.kind = classifyShape(o);
            double massOverride = 0.0;
            if (i < objectPhysics.size())
                massOverride = objectPhysics[i].massOverride;
            b.collisionRepr = collisionReprForObject(o);
            if (b.collisionRepr == CollisionRepr::Triangle)
                initBodyFromTriMesh(b, o, massOverride);
            else {
                initBodyFromPartSpheres(b, o, massOverride);
                b.collisionRepr = CollisionRepr::Sphere;
            }
            if (!bodyUsesRotation(b)) {
                if (massOverride > 1e-9)
                    initBodyFromPartSpheres(b, o, 0.0);
                b.invInertia = 0.0;
                b.angularVelocity = vec<>(0, 0, 0);
                b.spinDeg = 0.0;
            }
            b.anchor = getObjectAnchor(o);
            b.comOffset = b.baseCenter - b.anchor;
            if (const auto* bx = dynamic_cast<const EditorBox*>(o)) {
                b.halfExtents = vec<>(0.5 * std::abs(bx->dx * bx->scale.x), 0.5 * std::abs(bx->dy * bx->scale.y),
                                      0.5 * std::abs(bx->dz * bx->scale.z));
            } else if (const auto* cy = dynamic_cast<const EditorCylinder*>(o)) {
                b.cylRadius = std::abs(cy->baseRadius) * std::max(std::abs(cy->scale.x), std::abs(cy->scale.z));
                b.cylHalfHeight = 0.5 * std::abs(cy->height * cy->scale.y);
            } else if (const auto* to = dynamic_cast<const EditorTorus*>(o)) {
                b.torMinor = std::abs(to->innerR) * 0.5 * (std::abs(to->scale.x) + std::abs(to->scale.z));
                b.torMajor = std::abs(to->outerR) * 0.5 * (std::abs(to->scale.x) + std::abs(to->scale.z));
            }
            if (b.partsTriLocal.empty() && b.partsLocal.size() <= 1 && massOverride <= 1e-9) {
                b.mass = estimateMass(b);
                b.invMass = b.mass > 1e-9 ? 1.0 / b.mass : 0;
                b.inertia = 0.4 * b.mass * b.radius * b.radius;
                b.invInertia = b.inertia > 1e-9 ? 1.0 / b.inertia : 0;
            }
            if (i < objectPhysics.size())
                b.groupId = objectPhysics[i].groupId;
            if (auto* f4 = dynamic_cast<FourDWireFigure*>(o)) {
                b.is4D = true;
                b.hyperRadius = std::abs(f4->sizeParam) *
                                std::max({std::abs(f4->scale.x), std::abs(f4->scale.y), std::abs(f4->scale.z)});
                b.radius = b.hyperRadius * 1.5;
                b.mass = std::max(0.5, b.hyperRadius * b.hyperRadius * b.hyperRadius);
                b.invMass = b.mass > 1e-9 ? 1.0 / b.mass : 0;
                b.inertia = 0.4 * b.mass * b.radius * b.radius;
                b.invInertia = b.inertia > 1e-9 ? 1.0 / b.inertia : 0;
            }
            if (i < objectPhysics.size()) {
                b.kPos = objectPhysics[i].pk;
                b.kVel = objectPhysics[i].vk;
                if (!b.is4D) {
                    b.kVel = 0.0;
                } else if (auto* f4 = dynamic_cast<FourDWireFigure*>(o)) {
                    f4->kPos = b.kPos;
                }
                b.velocity = objectPhysics[i].velocity;
                b.useGravity = objectPhysics[i].useGravity;
                if (!b.useGravity) {
                    b.invMass = 0.0;
                    b.invInertia = 0.0;
                    b.velocity = vec<>(0, 0, 0);
                    b.angularVelocity = vec<>(0, 0, 0);
                }
                b.useFriction = objectPhysics[i].useFriction;
                b.collide = objectPhysics[i].collide;
                b.alpha = objectPhysics[i].alpha;
                b.gravity = objectPhysics[i].gravity;
                b.groundFriction = objectPhysics[i].groundFriction;
                b.restitution = objectPhysics[i].restitution;
                setFigureRenderAlpha(o, objectPhysics[i].alpha);
            } else {
                double seed = static_cast<double>(i + 1);
                b.velocity = vec<>(std::sin(seed * 0.7) * 0.8, 0.0, std::cos(seed * 0.9) * 0.8);
            }
            if (!prevObj.empty() && prevObj.size() == Objects.size() && prevObj[i] == o) {
                b.center = prevCenter[i];
                b.velocity = prevVel[i];
                b.kPos = prevK[i];
                b.kVel = prevKV[i];
            }
            bodies.push_back(b);
        }

        // Group aggregation: shared COM + aggregated mass/inertia on leader.
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].groupId < 0)
                continue;
            int gid = bodies[i].groupId;
            size_t leader = i;
            for (size_t j = 0; j < bodies.size(); ++j) {
                if (bodies[j].groupId == gid) {
                    leader = j;
                    break;
                }
            }
            double totalMass = 0.0;
            vec<> com(0, 0, 0);
            for (size_t j = 0; j < bodies.size(); ++j) {
                if (bodies[j].groupId != gid)
                    continue;
                totalMass += bodies[j].mass;
                com += bodies[j].center * bodies[j].mass;
            }
            if (totalMass > 1e-9)
                com = com / totalMass;
            else
                com = bodies[leader].center;
            if (gid >= static_cast<int>(groupCom.size()))
                groupCom.resize(static_cast<size_t>(gid + 1), com);
            groupCom[static_cast<size_t>(gid)] = com;

            double groupInertia = 0.0;
            for (size_t j = 0; j < bodies.size(); ++j) {
                if (bodies[j].groupId != gid)
                    continue;
                vec<> d = bodies[j].center - com;
                groupInertia += bodies[j].inertia + bodies[j].mass * d.len2();
            }

            for (size_t j = 0; j < bodies.size(); ++j) {
                if (bodies[j].groupId != gid)
                    continue;
                bodies[j].leader = static_cast<int>(leader);
                bodies[j].isLeader = (j == leader);
                bodies[j].localFromCom = bodies[j].center - com;
                if (!bodies[j].isLeader) {
                    bodies[j].invMass = 0.0;
                    bodies[j].invInertia = 0.0;
                } else {
                    bodies[j].mass = std::max(0.5, totalMass);
                    bodies[j].invMass = 1.0 / bodies[j].mass;
                    bodies[j].inertia = std::max(1e-6, groupInertia);
                    bodies[j].invInertia = 1.0 / bodies[j].inertia;
                    bodies[j].center = com;
                }
            }
        }
        physicsInitialized = true;
    }

    static bool bodyIsThinPlate(const BodyState& mesh)
    {
        if (!bodyUsesTriangleCollision(mesh))
            return false;
        const bool haveHe = mesh.halfExtents.x > 1e-9 && mesh.halfExtents.y > 1e-9 && mesh.halfExtents.z > 1e-9;
        if (!haveHe)
            return false;
        const double thinRatio =
            std::max(mesh.halfExtents.x, mesh.halfExtents.z) / std::max(0.05, mesh.halfExtents.y);
        return thinRatio >= 4.0;
    }

    /** Минимальный Y центра сферы: пол + верхние грани тонких плит под (x,z). */
    double supportCenterYForSphere(const BodyState& sph, size_t selfIdx) const
    {
        double minY = sph.radius;
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (j == selfIdx || !bodies[j].collide)
                continue;
            const BodyState& mesh = bodies[j];
            if (!bodyUsesTriangleCollision(mesh))
                continue;
            const bool haveHe = mesh.halfExtents.x > 1e-9 && mesh.halfExtents.y > 1e-9 &&
                                mesh.halfExtents.z > 1e-9;
            if (!haveHe)
                continue;
            const double thinRatio = std::max(mesh.halfExtents.x, mesh.halfExtents.z) /
                                     std::max(0.05, mesh.halfExtents.y);
            if (thinRatio < 4.0)
                continue;
            const double dx = std::max(0.0, std::abs(sph.center.x - mesh.center.x) - mesh.halfExtents.x);
            const double dz = std::max(0.0, std::abs(sph.center.z - mesh.center.z) - mesh.halfExtents.z);
            if (dx * dx + dz * dz > sph.radius * sph.radius)
                continue;
            const double topY = mesh.center.y + mesh.halfExtents.y;
            minY = std::max(minY, topY + sph.radius);
        }
        return minY;
    }

    /** Верхняя опора (Y) для меш-тела от тонких плит; 0 — только пол. */
    double supportSurfaceYForMesh(const BodyState& body, size_t selfIdx) const
    {
        double topSupport = 0.0;
        std::vector<CollTri> bodyTris;
        worldPartTriangles(body, physicsTime, bodyTris);
        if (bodyTris.empty())
            return 0.0;
        for (size_t j = 0; j < bodies.size(); ++j) {
            if (j == selfIdx || !bodies[j].collide || !bodyIsThinPlate(bodies[j]))
                continue;
            CollisionContact tmp;
            if (collision::meshBodyOnThinPlateTop(bodyTris, bodies[j].center, bodies[j].halfExtents, tmp))
                topSupport = std::max(topSupport, bodies[j].center.y + bodies[j].halfExtents.y);
        }
        return topSupport;
    }

    /** Гашение скорости только при контакте с опорой (не в воздухе). */
    static void calmBodyOnSupport(BodyState& b, double h, bool onSupport)
    {
        if (!onSupport)
            return;
        const double lin = std::pow(0.88, h * 60.0);
        b.velocity.x *= lin;
        b.velocity.z *= lin;
        if (std::abs(b.velocity.y) < 0.2)
            b.velocity.y = 0.0;
        if (b.velocity.len2() < 0.04 * 0.04)
            b.velocity = vec<>(0, 0, 0);
        if (bodyUsesRotation(b)) {
            b.angularVelocity = b.angularVelocity * std::pow(0.72, h * 60.0);
            if (b.angularVelocity.len2() < 0.03)
                b.angularVelocity = vec<>(0, 0, 0);
        }
    }

    bool detectCollision4D(int ia, int ib, Contact& c) const
    {
        const BodyState& a = bodies[ia];
        const BodyState& b = bodies[ib];
        if (!bodiesShareKSlice(a, b))
            return false;

        if (a.is4D && b.is4D) {
            fourd::HyperSphere ha{{a.center.x, a.center.y, a.center.z, a.kPos}, a.hyperRadius};
            fourd::HyperSphere hb{{b.center.x, b.center.y, b.center.z, b.kPos}, b.hyperRadius};
            Vec4 n4;
            double pen = 0;
            if (!fourd::hyperSphereSphereContact(ha, hb, n4, pen))
                return false;
            c.normal = vec<>(n4.x, n4.y, n4.z);
            c.penetration = pen;
            c.point = a.center + c.normal * a.hyperRadius;
            return true;
        }

        const int i4 = a.is4D ? ia : ib;
        const int i3 = a.is4D ? ib : ia;
        const BodyState& f4 = bodies[i4];
        const BodyState& s3 = bodies[i3];
        fourd::HyperSphere hs4{{f4.center.x, f4.center.y, f4.center.z, f4.kPos}, f4.hyperRadius};
        fourd::HyperSphere hs3{{s3.center.x, s3.center.y, s3.center.z, s3.kPos}, s3.radius};
        vec<> n3;
        double pen = 0;
        if (!fourd::hyperSphereProjected3DContact(hs4, hs3, camera4d, n3, pen))
            return false;
        c.normal = n3;
        c.penetration = pen;
        c.point = f4.center + n3 * f4.hyperRadius;
        return true;
    }

    void resolveCollision4D(int ia, int ib, const Contact& c)
    {
        BodyState& a = bodies[ia];
        BodyState& b = bodies[ib];
        resolveCollision(ia, ib, c);
        if (a.is4D && !b.is4D) {
            const double kn = (b.kPos - a.kPos);
            if (std::abs(kn) > 1e-9)
                b.kVel += kn * 0.12;
        } else if (!a.is4D && b.is4D) {
            const double kn = (a.kPos - b.kPos);
            if (std::abs(kn) > 1e-9)
                a.kVel += kn * 0.12;
        }
    }

    bool detectCollision(int ia, int ib, Contact& c) const {
        const BodyState& a = bodies[ia];
        const BodyState& b = bodies[ib];
        if (a.is4D || b.is4D)
            return false;
        if (!bodiesShareKSlice(a, b))
            return false;
        const bool meshA = bodyUsesTriangleCollision(a);
        const bool meshB = bodyUsesTriangleCollision(b);
        const bool compoundA = (a.kind == ShapeKind::Compound || a.kind == ShapeKind::Other);
        const bool compoundB = (b.kind == ShapeKind::Compound || b.kind == ShapeKind::Other);
        if (!meshA && !meshB && !compoundA && !compoundB) {
            vec<> d = b.center - a.center;
            double rr = a.radius + b.radius;
            if (d.len2() > rr * rr)
                return false;
        } else if (meshA || meshB) {
            vec<> d = b.center - a.center;
            double rr = a.radius + b.radius;
            if (d.len2() > rr * rr * 4.0)
                return false;
        }

        auto sphereSphere = [&](const BodyState& s0, const BodyState& s1, Contact& out) {
            vec<> dv = s1.center - s0.center;
            double dist = safeLen(dv);
            vec<> n = dist > 1e-8 ? dv * (1.0 / dist) : vec<>(1, 0, 0);
            out.normal = n;
            out.penetration = (s0.radius + s1.radius) - dist;
            out.point = s0.center + n * s0.radius;
            return out.penetration > 0;
        };

        auto sphereBox = [&](const BodyState& s, const BodyState& box, Contact& out) {
            vec<> q(std::clamp(s.center.x, box.center.x - box.halfExtents.x, box.center.x + box.halfExtents.x),
                    std::clamp(s.center.y, box.center.y - box.halfExtents.y, box.center.y + box.halfExtents.y),
                    std::clamp(s.center.z, box.center.z - box.halfExtents.z, box.center.z + box.halfExtents.z));
            vec<> v = s.center - q;
            double d2 = v.len2();
            if (d2 > s.radius * s.radius) return false;
            double d = std::sqrt(std::max(0.0, d2));
            out.normal = d > 1e-8 ? v * (1.0 / d) : vec<>(0, 1, 0);
            out.penetration = s.radius - d;
            out.point = q;
            return true;
        };

        auto sphereCylinder = [&](const BodyState& s, const BodyState& cy, Contact& out) {
            vec<> rel = s.center - cy.center;
            double y = std::clamp(rel.y, -cy.cylHalfHeight, cy.cylHalfHeight);
            vec<> radial(rel.x, 0, rel.z);
            double rl = safeLen(radial);
            if (rl > cy.cylRadius) radial = radial * (cy.cylRadius / rl);
            vec<> q = cy.center + vec<>(radial.x, y, radial.z);
            vec<> v = s.center - q;
            double d2 = v.len2();
            if (d2 > s.radius * s.radius) return false;
            double d = std::sqrt(std::max(0.0, d2));
            out.normal = d > 1e-8 ? v * (1.0 / d) : vec<>(0, 1, 0);
            out.penetration = s.radius - d;
            out.point = q;
            return true;
        };

        auto sphereTorus = [&](const BodyState& s, const BodyState& t, Contact& out) {
            vec<> rel = s.center - t.center;
            double radial = std::sqrt(rel.x * rel.x + rel.z * rel.z);
            double ring = std::max(1e-8, radial);
            vec<> tube = t.center + vec<>(rel.x / ring * t.torMajor, 0.0, rel.z / ring * t.torMajor);
            vec<> dv = s.center - tube;
            double d2 = dv.len2();
            double r = s.radius + t.torMinor;
            if (d2 > r * r) return false;
            double d = std::sqrt(std::max(0.0, d2));
            out.normal = d > 1e-8 ? dv * (1.0 / d) : vec<>(1, 0, 0);
            out.penetration = r - d;
            out.point = tube + out.normal * t.torMinor;
            return true;
        };

        auto collectSpheres = [&](const BodyState& bs, std::vector<std::pair<vec<>, double>>& out) {
            worldPartSpheres(bs, physicsTime, out);
        };
        auto collectTriangles = [&](const BodyState& bs, std::vector<CollTri>& out) {
            worldPartTriangles(bs, physicsTime, out);
        };

        auto sphereVsMesh = [&](const vec<>& sc, double sr, const BodyState& mesh, CollisionContact& cc) -> bool {
            CollisionContact best;
            best.penetration = -1.0;
            bool any = false;
            const bool haveHe =
                mesh.halfExtents.x > 1e-9 && mesh.halfExtents.y > 1e-9 && mesh.halfExtents.z > 1e-9;
            const double thinRatio = haveHe ? std::max(mesh.halfExtents.x, mesh.halfExtents.z) /
                                                  std::max(0.05, mesh.halfExtents.y)
                                            : 0.0;
            const bool thinPlate = thinRatio >= 4.0;

            CollisionContact tmp;
            if (thinPlate) {
                if (collision::sphereThinPlateTopContact(sc, sr, mesh.center, mesh.halfExtents, tmp) &&
                    tmp.penetration > 0.0) {
                    best = tmp;
                    any = true;
                }
            } else {
                if (haveHe && collision::sphereAabbContact(sc, sr, mesh.center, mesh.halfExtents, tmp) &&
                    tmp.penetration > 0.0) {
                    best = tmp;
                    any = true;
                }
                std::vector<CollTri> tris;
                worldPartTriangles(mesh, physicsTime, tris);
                CollisionContact triHit;
                if (collision::bestSphereTriangleContact(sc, sr, tris, triHit, &mesh.center) &&
                    triHit.penetration > 0.0) {
                    if (!any || triHit.penetration > best.penetration) {
                        best = triHit;
                        any = true;
                    }
                }
            }
            if (!any)
                return false;
            cc = best;
            return true;
        };

        std::vector<std::pair<vec<>, double>> as, bs;
        std::vector<CollTri> at, bt;
        if (!meshA)
            collectSpheres(a, as);
        else
            collectTriangles(a, at);
        if (!meshB)
            collectSpheres(b, bs);
        else
            collectTriangles(b, bt);

        if (!meshA && as.empty())
            as.push_back({a.center, a.radius});
        if (!meshB && bs.empty())
            bs.push_back({b.center, b.radius});
        if (meshA && at.empty())
            return false;
        if (meshB && bt.empty())
            return false;

        Contact best;
        best.penetration = -1.0;
        bool hit = false;

        auto take = [&](const CollisionContact& cc) {
            if (cc.penetration > best.penetration) {
                best.point = cc.point;
                best.normal = cc.normal;
                best.penetration = cc.penetration;
                hit = true;
            }
        };
        auto takeMesh = [&](const CollisionContact& cc) {
            const double reach = a.radius + b.radius + 0.35;
            if ((cc.point - a.center).len() > reach || (cc.point - b.center).len() > reach)
                return;
            if (cc.penetration > 2.5)
                return;
            take(cc);
        };

        if (!meshA && !meshB) {
            for (const auto& sa : as) {
                for (const auto& sb : bs) {
                    BodyState ta = a, tb = b;
                    ta.center = sa.first;
                    ta.radius = sa.second;
                    tb.center = sb.first;
                    tb.radius = sb.second;
                    Contact tmp;
                    if (sphereSphere(ta, tb, tmp))
                        take({tmp.point, tmp.normal, tmp.penetration});
                }
            }
        } else if (!meshA && meshB) {
            for (const auto& sa : as) {
                CollisionContact tmp;
                if (sphereVsMesh(sa.first, sa.second, b, tmp))
                    take(tmp);
            }
        } else if (meshA && !meshB) {
            for (const auto& sb : bs) {
                CollisionContact tmp;
                if (sphereVsMesh(sb.first, sb.second, a, tmp)) {
                    tmp.normal = tmp.normal * -1.0;
                    take(tmp);
                }
            }
        } else {
            int plateIdx = -1, bodyIdx = -1;
            if (bodyIsThinPlate(a)) {
                plateIdx = ia;
                bodyIdx = ib;
            } else if (bodyIsThinPlate(b)) {
                plateIdx = ib;
                bodyIdx = ia;
            }
            if (plateIdx >= 0) {
                std::vector<CollTri> bodyTris;
                worldPartTriangles(bodies[bodyIdx], physicsTime, bodyTris);
                CollisionContact tmp;
                if (collision::meshBodyOnThinPlateTop(bodyTris, bodies[plateIdx].center,
                                                      bodies[plateIdx].halfExtents, tmp))
                    take(tmp);
            } else {
                /* Меш vs меш: вершина–треугольник (triangleTriangleContact), нормаль вдоль a→b. */
                const vec<> sepAB = b.center - a.center;
                const size_t stepA = std::max(size_t(1), at.size() / 40);
                const size_t stepB = std::max(size_t(1), bt.size() / 40);
                for (size_t ia = 0; ia < at.size(); ia += stepA) {
                    for (size_t ib = 0; ib < bt.size(); ib += stepB) {
                        CollisionContact tmp;
                        if (!collision::triangleTriangleContact(at[ia], bt[ib], tmp))
                            continue;
                        if (tmp.normal.dot(sepAB) < 0.0)
                            tmp.normal = tmp.normal * -1.0;
                        takeMesh(tmp);
                    }
                }
            }
        }

        if (hit) {
            c = best;
            return true;
        }

        if (compoundA || compoundB)
            return false;

        if (!meshA && !meshB) {
            if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Sphere)
                return sphereSphere(a, b, c);
            if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Box)
                return sphereBox(a, b, c);
            if (a.kind == ShapeKind::Box && b.kind == ShapeKind::Sphere) {
                bool ok = sphereBox(b, a, c);
                c.normal = c.normal * -1.0;
                return ok;
            }
            if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Cylinder)
                return sphereCylinder(a, b, c);
            if (a.kind == ShapeKind::Cylinder && b.kind == ShapeKind::Sphere) {
                bool ok = sphereCylinder(b, a, c);
                c.normal = c.normal * -1.0;
                return ok;
            }
            if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Torus)
                return sphereTorus(a, b, c);
            if (a.kind == ShapeKind::Torus && b.kind == ShapeKind::Sphere) {
                bool ok = sphereTorus(b, a, c);
                c.normal = c.normal * -1.0;
                return ok;
            }
            return sphereSphere(a, b, c);
        }

        return false;
    }

    void resolveCollision(int ia, int ib, const Contact& c) {
        BodyState& a = bodies[ia];
        BodyState& b = bodies[ib];
        vec<> n = safeNorm(c.normal, vec<>(1, 0, 0));
        vec<> ra = c.point - a.center;
        vec<> rb = c.point - b.center;
        vec<> va = a.velocity + (a.angularVelocity ^ ra);
        vec<> vb = b.velocity + (b.angularVelocity ^ rb);
        vec<> rv = vb - va;
        double vn = rv.dot(n);

        double raCrossN2 = (ra ^ n).len2();
        double rbCrossN2 = (rb ^ n).len2();
        double denom = a.invMass + b.invMass + raCrossN2 * a.invInertia + rbCrossN2 * b.invInertia;
        if (vn < 0.0 && denom > 1e-12) {
            double restitution = std::min(a.restitution, b.restitution);
            restitution = std::min(restitution, 0.08);
            if (std::abs(vn) < 0.45)
                restitution = 0.0;
            double jn = -(1.0 + restitution) * vn / denom;
            vec<> impulse = n * jn;
            a.velocity -= impulse * a.invMass;
            b.velocity += impulse * b.invMass;
            if (a.invInertia > 1e-12)
                a.angularVelocity -= (ra ^ impulse) * a.invInertia;
            if (b.invInertia > 1e-12)
                b.angularVelocity += (rb ^ impulse) * b.invInertia;

            vec<> tangent = rv - n * vn;
            tangent = safeNorm(tangent, vec<>(0, 0, 1));
            double vt = rv.dot(tangent);
            double jt = -vt / denom;
            double mu = 0.42;
            vec<> frImpulse = tangent * std::clamp(jt, -mu * jn, mu * jn);
            a.velocity -= frImpulse * a.invMass;
            b.velocity += frImpulse * b.invMass;
        }

        const double slop = 0.008;
        const double corrK = c.penetration < 0.04 ? 0.28 : 0.55;
        double corr = std::max(0.0, c.penetration - slop) * corrK / std::max(1e-9, a.invMass + b.invMass);
        corr = std::min(corr, std::min(0.45, c.penetration * 0.85));
        vec<> corrVec = n * corr;
        a.center -= corrVec * a.invMass;
        b.center += corrVec * b.invMass;
    }

    void stepPhysics(double dt) {
        if (!physicsInitialized || bodies.size() != Objects.size())
            rebuildBodies();
        if (bodies.empty())
            return;

        const int substeps = 4;
        const double h = dt / substeps;
        for (int step = 0; step < substeps; ++step) {
            for (size_t i = 0; i < bodies.size(); ++i) {
                BodyState& b = bodies[i];
                if (!b.isLeader && b.groupId >= 0)
                    continue;
                if (b.useGravity)
                    b.velocity += b.gravity * h;
                if (bodyUsesRotation(b))
                    b.angularVelocity = b.angularVelocity * std::pow(0.995, h * 60.0);
                else
                    b.angularVelocity = vec<>(0, 0, 0);
                b.center += b.velocity * h;
                if (i < objectPhysics.size() && std::abs(objectPhysics[i].orbitOmegaY) > 1e-9) {
                    const ObjectPhysics& p = objectPhysics[i];
                    vec<> rel = b.center - p.orbitCenter;
                    if (rel.len2() < 1e-8)
                        rel = b.baseCenter - p.orbitCenter;
                    if (rel.len2() < 1e-8)
                        rel = vec<>(1.0, 0.0, 0.0);
                    const double omega = p.orbitOmegaY * M_PI / 180.0;
                    const double ang = omega * h;
                    b.velocity.x = omega * rel.z;
                    b.velocity.z = -omega * rel.x;
                    vec<> rel2(std::cos(ang) * rel.x + std::sin(ang) * rel.z, rel.y,
                              -std::sin(ang) * rel.x + std::cos(ang) * rel.z);
                    b.center = p.orbitCenter + rel2;
                }

                if (bodyUsesRotation(b)) {
                    double w = safeLen(b.angularVelocity);
                    if (w > 1e-8) {
                        b.spinAxis = b.angularVelocity * (1.0 / w);
                        b.spinDeg += (w * h) * 180.0 / M_PI;
                        if (b.spinDeg > 3600.0)
                            b.spinDeg = std::fmod(b.spinDeg, 360.0);
                    }
                    double wcap = safeLen(b.angularVelocity);
                    if (wcap > 25.0)
                        b.angularVelocity = b.angularVelocity * (25.0 / wcap);
                } else {
                    b.spinDeg = 0.0;
                }
                if (b.is4D)
                    b.kPos += b.kVel * h;

            }

            for (int pass = 0; pass < 2; ++pass) {
            for (int i = 0; i < static_cast<int>(bodies.size()); ++i)
                for (int j = i + 1; j < static_cast<int>(bodies.size()); ++j) {
                    if (bodies[i].groupId >= 0 && bodies[i].groupId == bodies[j].groupId)
                        continue;
                    if (!bodies[i].isLeader && bodies[i].groupId >= 0)
                        continue;
                    if (!bodies[j].isLeader && bodies[j].groupId >= 0)
                        continue;
                    if (!bodies[i].collide || !bodies[j].collide)
                        continue;
                    Contact c;
                    bool hit = false;
                    if (bodies[i].is4D || bodies[j].is4D)
                        hit = detectCollision4D(i, j, c);
                    else
                        hit = detectCollision(i, j, c);
                    const bool compoundPair =
                        (bodies[i].kind == ShapeKind::Compound || bodies[i].kind == ShapeKind::Other ||
                         bodies[j].kind == ShapeKind::Compound || bodies[j].kind == ShapeKind::Other);
                    const bool meshPair = bodyUsesTriangleCollision(bodies[i]) || bodyUsesTriangleCollision(bodies[j]);
                    if (!hit && !compoundPair && !meshPair) {
                        // Swept sphere fallback for fast motion to reduce tunneling (simple shapes only).
                        BodyState& a = bodies[i];
                        BodyState& b = bodies[j];
                        vec<> rv = (b.velocity - a.velocity) * h;
                        vec<> d = b.center - a.center;
                        double rr = a.radius + b.radius;
                        double A = rv.len2();
                        double B = 2.0 * d.dot(rv);
                        double C = d.len2() - rr * rr;
                        if (A > 1e-12) {
                            double disc = B * B - 4.0 * A * C;
                            if (disc >= 0.0) {
                                double tcol = (-B - std::sqrt(disc)) / (2.0 * A);
                                if (tcol >= 0.0 && tcol <= 1.0) {
                                    vec<> ca = a.center + a.velocity * (h * tcol);
                                    vec<> cb = b.center + b.velocity * (h * tcol);
                                    vec<> n = safeNorm(cb - ca, vec<>(1, 0, 0));
                                    c.normal = n;
                                    c.point = ca + n * a.radius;
                                    c.penetration = 0.001;
                                    hit = true;
                                }
                            }
                        }
                    }
                    if (!hit) {
                        const bool meshPair =
                            bodyUsesTriangleCollision(bodies[i]) || bodyUsesTriangleCollision(bodies[j]);
                        int si = -1, mi = -1;
                        if (!bodyUsesTriangleCollision(bodies[i]) && bodies[i].collisionRepr == CollisionRepr::Sphere) {
                            si = i;
                            mi = j;
                        } else if (!bodyUsesTriangleCollision(bodies[j]) &&
                                   bodies[j].collisionRepr == CollisionRepr::Sphere) {
                            si = j;
                            mi = i;
                        }
                        if (meshPair && si >= 0 && bodyUsesTriangleCollision(bodies[mi])) {
                            const BodyState& sph = bodies[si];
                            const BodyState& mesh = bodies[mi];
                            const bool haveHe = mesh.halfExtents.x > 1e-9 && mesh.halfExtents.y > 1e-9 &&
                                                mesh.halfExtents.z > 1e-9;
                            const double thinRatio = haveHe
                                                         ? std::max(mesh.halfExtents.x, mesh.halfExtents.z) /
                                                               std::max(0.05, mesh.halfExtents.y)
                                                         : 0.0;
                            if (thinRatio >= 4.0) {
                                const vec<> p0 = sph.center - sph.velocity * h;
                                CollisionContact tmp;
                                if (collision::sphereThinPlateTopSwept(p0, sph.center, sph.radius, mesh.center,
                                                                     mesh.halfExtents, tmp)) {
                                    c.point = tmp.point;
                                    c.normal = tmp.normal;
                                    c.penetration = tmp.penetration;
                                    hit = true;
                                }
                            }
                        }
                    }
                    if (hit) {
                        if (bodies[i].is4D || bodies[j].is4D)
                            resolveCollision4D(i, j, c);
                        else
                            resolveCollision(i, j, c);
                    }
                }
            }

            for (size_t i = 0; i < bodies.size(); ++i) {
                BodyState& b = bodies[i];
                if (!b.isLeader && b.groupId >= 0)
                    continue;
                if (!b.collide)
                    continue;

                const double boundR = b.radius;
                double minX = -95.0 + boundR, maxX = 95.0 - boundR;
                double minZ = -95.0 + boundR, maxZ = 95.0 - boundR;
                if (b.center.x < minX) { b.center.x = minX; b.velocity.x = std::abs(b.velocity.x); }
                else if (b.center.x > maxX) { b.center.x = maxX; b.velocity.x = -std::abs(b.velocity.x); }
                if (b.center.z < minZ) { b.center.z = minZ; b.velocity.z = std::abs(b.velocity.z); }
                else if (b.center.z > maxZ) { b.center.z = maxZ; b.velocity.z = -std::abs(b.velocity.z); }

                constexpr double groundY = 0.0;
                bool onSupport = false;
                if (bodyUsesTriangleCollision(b)) {
                    const double plateTop = supportSurfaceYForMesh(b, i);
                    const double lowest = bodyLowestY(b, physicsTime);
                    const double floorY = plateTop > 1e-6 ? plateTop : groundY;
                    if (lowest < floorY + 0.02) {
                        if (lowest < floorY)
                            b.center.y += floorY - lowest;
                        onSupport = true;
                        if (b.velocity.y < 0.0)
                            b.velocity.y = 0.0;
                        if (b.useFriction) {
                            const double f = std::clamp(1.0 - b.groundFriction * h * 2.0, 0.0, 1.0);
                            b.velocity.x *= f;
                            b.velocity.z *= f;
                        }
                    }
                } else {
                    const double supportY = supportCenterYForSphere(b, i);
                    if (b.center.y <= supportY + 0.02) {
                        if (b.center.y < supportY - 1e-6)
                            b.center.y = supportY;
                        onSupport = true;
                        if (b.velocity.y < 0.0)
                            b.velocity.y = 0.0;
                        if (b.useFriction) {
                            const double f = std::clamp(1.0 - b.groundFriction * h * 2.0, 0.0, 1.0);
                            b.velocity.x *= f;
                            b.velocity.z *= f;
                        }
                    } else {
                        const double bottom = b.center.y - boundR;
                        if (bottom < groundY + 0.02) {
                            if (bottom < groundY)
                                b.center.y = boundR;
                            onSupport = true;
                            if (b.velocity.y < 0.0)
                                b.velocity.y = 0.0;
                            if (b.useFriction) {
                                const double f = std::clamp(1.0 - b.groundFriction * h * 2.0, 0.0, 1.0);
                                b.velocity.x *= f;
                                b.velocity.z *= f;
                            }
                        }
                    }
                }
                calmBodyOnSupport(b, h, onSupport);
            }
        }

        for (size_t i = 0; i < bodies.size(); ++i) {
            int gid = bodies[i].groupId;
            if (gid < 0)
                continue;
            const int leader = bodies[i].leader;
            if (leader < 0 || static_cast<size_t>(leader) >= bodies.size())
                continue;
            if (static_cast<size_t>(leader) == i)
                continue;
            const BodyState& l = bodies[static_cast<size_t>(leader)];
            vec<> rotLocal = rotateAroundAxis(bodies[i].localFromCom, l.spinAxis, l.spinDeg * M_PI / 180.0);
            bodies[i].center = l.center + rotLocal;
            bodies[i].velocity = l.velocity;
            bodies[i].angularVelocity = l.angularVelocity;
            bodies[i].spinAxis = l.spinAxis;
            bodies[i].spinDeg = l.spinDeg;
        }
    }
    
    void Render(double t = (double)clock() / CLOCKS_PER_SEC) {
        glPushMatrix();
        if (!physicsInitialized) {
            rebuildBodies();
            physicsTime = t;
        }
        double dt = std::clamp(t - physicsTime, 0.0, 1.0 / 30.0) * std::clamp(physicsTimeScale, 0.2, 3.0);
        physicsTime = t;
        stepPhysics(dt);

        double proj[16], viewMat[16];
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetDoublev(GL_MODELVIEW_MATRIX, viewMat);
        

        double planes[6][4];
        extractFrustumPlanesFromProj(planes, proj);

        updateSceneLight(physicsCameraPos);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        int drawnCount = 0;
        if (envSky) {
            glDisable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
            envSky->Draw(t);
            ++drawnCount;
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);

        for (size_t i = 0; i < Objects.size(); ++i) {
            based* el = Objects[i];
            std::vector<std::pair<vec<>, double>> parts;
            if (i < bodies.size())
                worldPartSpheres(bodies[i], t, parts);
            else {
                el->getBoundingSpheres(parts, t);
                if (parts.empty()) {
                    vec<> c;
                    double r = 0;
                    el->emergency_bounding_sphere_calc_protocol(c, r, t);
                    parts.push_back({c, r});
                }
            }

            bool visible = false;
            if (dynamic_cast<FourDWireFigure*>(el) && use4dCamera)
                visible = true;
            for (const auto& pr : parts) {
                vec<> eyeCenter = transformPoint(pr.first, viewMat);
                if (sphereInFrustum(planes, eyeCenter, pr.second)) {
                    visible = true;
                    break;
                }
            }

            if (visible) {
                double drawAlpha = 1.0;
                if (i < bodies.size())
                    drawAlpha = bodies[i].alpha;
                const AlphaReflect ar = decomposeAlphaReflect(drawAlpha);
                setFigureRenderAlpha(el, drawAlpha);
                applyFigureMaterial(ar.opacity, ar.reflect);
                const bool transparent = ar.opacity < 0.999;
                if (transparent)
                    glDepthMask(GL_FALSE);
                if (auto* f4 = dynamic_cast<FourDWireFigure*>(el)) {
                    const double kW = (i < bodies.size()) ? bodies[i].kPos : f4->kPos;
                    if (use4dCamera)
                        f4->drawProjected(camera4d, kW);
                    else if (i < bodies.size())
                        drawObjectRigidBody(el, bodies[i], t);
                    else
                        f4->drawProjected(camera4d, kW);
                } else if (i < bodies.size())
                    drawObjectRigidBody(el, bodies[i], t);
                else
                    el->Draw(t);
                if (transparent)
                    glDepthMask(GL_TRUE);
                drawnCount++;
            }
        }

        if (envGround) {
            glDisable(GL_LIGHTING);
            glEnable(GL_TEXTURE_2D);
            envGround->Draw(t);
            ++drawnCount;
            glEnable(GL_LIGHTING);
        }

        if (use4dCamera)
            draw4dAxes();

        glDisable(GL_BLEND);
        lastDrawnCount = drawnCount;

        if (debugLayer == 1)
            drawDebugBoundingSpheres(t);
        else if (debugLayer == 2)
            drawDebugDynamics(t);

        glPopMatrix();
    }

    void draw4dAxes() const {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glLineWidth(2.0f);
        auto axis = [&](float r, float g, float b, const Vec4& tip4) {
            vec<> tip3;
            if (!fourd::projectTo3D(camera4d, tip4, tip3))
                return;
            glColor3f(r, g, b);
            glBegin(GL_LINES);
            vec<> o;
            if (fourd::projectTo3D(camera4d, {0, 0, 0, 0}, o)) {
                glVertex3d(o.x, o.y, o.z);
                glVertex3d(tip3.x, tip3.y, tip3.z);
            }
            glEnd();
        };
        axis(1.0f, 0.25f, 0.25f, {4, 0, 0, 0});
        axis(0.25f, 1.0f, 0.25f, {0, 4, 0, 0});
        axis(0.25f, 0.25f, 1.0f, {0, 0, 4, 0});
        axis(1.0f, 0.0f, 1.0f, {0, 0, 0, 4});
        glPopAttrib();
    }

    void drawDebugDynamics(double t) const {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glLineWidth(2.0f);
        const double horizon = 5.0;
        for (size_t i = 0; i < bodies.size(); ++i) {
            const BodyState& b = bodies[i];
            if (!b.isLeader && b.groupId >= 0)
                continue;
            glColor3f(1.0f, 0.85f, 0.1f);
            glPushMatrix();
            glTranslated(b.center.x, b.center.y, b.center.z);
            glutWireSphere(std::max(0.06, b.radius * 0.08), 10, 8);
            glPopMatrix();
            const vec<> v = b.velocity;
            if (v.len2() > 1e-8) {
                glColor3f(0.95f, 0.35f, 0.1f);
                const vec<> tip = b.center + v;
                glBegin(GL_LINES);
                glVertex3d(b.center.x, b.center.y, b.center.z);
                glVertex3d(tip.x, tip.y, tip.z);
                glEnd();
                glColor3f(0.2f, 0.75f, 1.0f);
                const vec<> end = b.center + v * horizon;
                glBegin(GL_LINES);
                glVertex3d(b.center.x, b.center.y, b.center.z);
                glVertex3d(end.x, end.y, end.z);
                glEnd();
            }
        }
        (void)t;
        glPopAttrib();
    }

    void drawDebugBoundingSpheres(double t) const {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDepthMask(GL_FALSE);
        glLineWidth(1.5f);

        for (size_t i = 0; i < Objects.size(); ++i) {
            const bool mesh = i < bodies.size() && bodyUsesTriangleCollision(bodies[i]);
            if (mesh) {
                std::vector<CollTri> wt;
                worldPartTrianglesLod(bodies[i], collisionSubdivForBody(bodies[i]), wt);
                glColor3f(0.2f, 0.95f, 0.35f);
                for (const CollTri& tri : wt) {
                    glBegin(GL_LINE_LOOP);
                    glVertex3d(tri.v0.x, tri.v0.y, tri.v0.z);
                    glVertex3d(tri.v1.x, tri.v1.y, tri.v1.z);
                    glVertex3d(tri.v2.x, tri.v2.y, tri.v2.z);
                    glEnd();
                }
            } else {
                std::vector<std::pair<vec<>, double>> parts;
                if (i < bodies.size())
                    worldPartSpheres(bodies[i], t, parts);
                else {
                    Objects[i]->getBoundingSpheres(parts, t);
                    if (parts.empty()) {
                        vec<> c;
                        double r = 0;
                        Objects[i]->emergency_bounding_sphere_calc_protocol(c, r, t);
                        parts.push_back({c, r});
                    }
                }
                for (const auto& pr : parts) {
                    glColor3f(0.2f, 0.95f, 0.35f);
                    glPushMatrix();
                    glTranslated(pr.first.x, pr.first.y, pr.first.z);
                    glutWireSphere(std::max(0.05, pr.second), 12, 10);
                    glPopMatrix();
                }
            }
        }

        glDepthMask(GL_TRUE);
        glPopAttrib();
    }
};
