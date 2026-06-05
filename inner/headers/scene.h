#pragma once

#include "figures.h"
#include "manual_shapes.h"
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
    struct BodyState {
        based* obj = nullptr;
        ShapeKind kind = ShapeKind::Other;
        vec<> baseCenter;
        vec<> center;
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
        vec<> gravity = vec<>(0, -9.81, 0);
        double groundFriction = 0.0;
        double restitution = 0.74;
        int leader = -1;
        bool isLeader = true;
        vec<> localFromCom;
    };

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
        vec<> gravity = vec<>(0, -9.81, 0);
        double groundFriction = 0.0;
        double restitution = 0.74;
    };

    std::vector<based*> Objects;
    std::vector<ObjectPhysics> objectPhysics;
    std::vector<based*> texturedObjects;
    std::vector<std::function<based*()>> objectList;
    std::vector<std::string> texturePaths;
    std::vector<BodyState> bodies;
    std::vector<vec<>> groupCom;
    double physicsTime = 0.0;
    bool physicsInitialized = false;

public:
    unsigned int lastDrawnCount = 0;
    Scene(void)
    {
        texturedObjects.push_back(new GroundPlane(LoadTexID("textures/water.png")));
        texturedObjects.push_back(new SkySphere(LoadTexID("textures/mountains.jpg")));

        // KABASIK START
//        Objects.push_back(new kabasik(vec<>(0, 5, 0), vec<>(0, 0, 1), 1, 30, 0, vec<>(5, 0, 5)));
//        Objects.push_back(new kabasik(vec<>(0, 5, 20), vec<>(0, 0, 1), 1, 30, 0, vec<>(5, 0, 5)));
//        Objects.push_back(new kabasik(vec<>(20, 5 * 1.5, 20), vec<>(0, 0, 1), 1.5, 30, 0, vec<>(5, 0, 5)));

        for (int i = 0; i < 0; i++) {
            based* k = new kabasik();
            k->setTexture(LoadTexID("textures/battler.png"));
            Objects.push_back(k);
        }

        // KABASIK END

        // TREES START
        // there is a problem with bounding sphere
        for (int i = 0; i < 0; i++)
            Objects.push_back(new tree());
        // TREES END

        // SHOW START
        for (int i = 0; i < 0; i++)
            Objects.push_back(new snowflake());
        // SNOW END

        // KANAR
        for (int i = 0; i < 0; i++) {
            based* k = new kanar();
            k->setTexture(LoadTexID("textures/Filth.png"));
            Objects.push_back(k);
        }
        // END KANAR

        // SNOWMAN
        for (int i = 0; i < 0; i++)
            Objects.push_back(new snowman());
        //END SNOWMAN
        
        objectList.push_back([]() -> based* { return new kabasik(); });
        texturePaths.push_back("textures/battler.png");
        objectList.push_back([]() -> based* { return new tree(); });
        texturePaths.push_back("textures/Goddess.png");
        objectList.push_back([]() -> based* { return new snowflake(); });
        texturePaths.push_back("textures/eldrich_horror.png");
        objectList.push_back([]() -> based* { return new kanar(); });
        texturePaths.push_back("textures/Filth.png");
        objectList.push_back([]() -> based* { return new snowman(); });
        texturePaths.push_back("textures/poch.png");
    }
    
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

    static ShapeKind classifyShape(const based* obj) {
        if (dynamic_cast<const EditorSphere*>(obj)) return ShapeKind::Sphere;
        if (dynamic_cast<const EditorBox*>(obj)) return ShapeKind::Box;
        if (dynamic_cast<const EditorCylinder*>(obj)) return ShapeKind::Cylinder;
        if (dynamic_cast<const EditorTorus*>(obj)) return ShapeKind::Torus;
        if (dynamic_cast<const TransformWrapper*>(obj)) return ShapeKind::Compound;
        return ShapeKind::Other;
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
        bodies.clear();
        bodies.reserve(Objects.size());
        groupCom.clear();
        for (size_t i = 0; i < Objects.size(); ++i) {
            based* o = Objects[i];
            BodyState b;
            b.obj = o;
            b.kind = classifyShape(o);
            o->getBoundingSphere(b.center, b.radius, 0);
            b.baseCenter = b.center;
            b.radius = std::max(0.1, b.radius);
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
            b.mass = estimateMass(b);
            b.invMass = b.mass > 1e-9 ? 1.0 / b.mass : 0;
            b.inertia = 0.4 * b.mass * b.radius * b.radius;
            b.invInertia = b.inertia > 1e-9 ? 1.0 / b.inertia : 0;
            if (i < objectPhysics.size())
                b.groupId = objectPhysics[i].groupId;
            if (i < objectPhysics.size()) {
                b.velocity = objectPhysics[i].velocity;
                b.useGravity = objectPhysics[i].useGravity;
                b.useFriction = objectPhysics[i].useFriction;
                b.gravity = objectPhysics[i].gravity;
                b.groundFriction = objectPhysics[i].groundFriction;
                b.restitution = objectPhysics[i].restitution;
            } else {
                double seed = static_cast<double>(i + 1);
                b.velocity = vec<>(std::sin(seed * 0.7) * 0.8, 0.0, std::cos(seed * 0.9) * 0.8);
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

    bool detectCollision(int ia, int ib, Contact& c) const {
        const BodyState& a = bodies[ia];
        const BodyState& b = bodies[ib];
        vec<> d = b.center - a.center;
        double rr = a.radius + b.radius;
        if (d.len2() > rr * rr) return false;

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
            out.clear();
            if (bs.obj)
                bs.obj->collectBoundingSpheres(out, physicsTime);
            if (out.empty())
                out.push_back({bs.center, bs.radius});
            const vec<> d = bs.center - bs.baseCenter;
            for (auto& s : out) {
                s.first = s.first + d;
                s.first = s.first - bs.center;
                s.first = rotateAroundAxis(s.first, bs.spinAxis, bs.spinDeg * M_PI / 180.0);
                s.first = s.first + bs.center;
            }
        };

        {
            std::vector<std::pair<vec<>, double>> as, bs;
            collectSpheres(a, as);
            collectSpheres(b, bs);
            bool hit = false;
            Contact best;
            best.penetration = -1.0;
            for (const auto& sa : as) {
                for (const auto& sb : bs) {
                    BodyState ta = a, tb = b;
                    ta.center = sa.first; ta.radius = sa.second;
                    tb.center = sb.first; tb.radius = sb.second;
                    Contact tmp;
                    if (sphereSphere(ta, tb, tmp) && tmp.penetration > best.penetration) {
                        best = tmp;
                        hit = true;
                    }
                }
            }
            if (hit) c = best;
            if (hit)
                return true;
        }

        if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Sphere) return sphereSphere(a, b, c);
        if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Box) return sphereBox(a, b, c);
        if (a.kind == ShapeKind::Box && b.kind == ShapeKind::Sphere) { bool ok = sphereBox(b, a, c); c.normal = -c.normal; return ok; }
        if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Cylinder) return sphereCylinder(a, b, c);
        if (a.kind == ShapeKind::Cylinder && b.kind == ShapeKind::Sphere) { bool ok = sphereCylinder(b, a, c); c.normal = -c.normal; return ok; }
        if (a.kind == ShapeKind::Sphere && b.kind == ShapeKind::Torus) return sphereTorus(a, b, c);
        if (a.kind == ShapeKind::Torus && b.kind == ShapeKind::Sphere) { bool ok = sphereTorus(b, a, c); c.normal = -c.normal; return ok; }

        return sphereSphere(a, b, c);
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
            const double restitution = std::min(a.restitution, b.restitution);
            double jn = -(1.0 + restitution) * vn / denom;
            vec<> impulse = n * jn;
            a.velocity -= impulse * a.invMass;
            b.velocity += impulse * b.invMass;
            a.angularVelocity -= (ra ^ impulse) * a.invInertia;
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

        double corr = std::max(0.0, c.penetration - 0.001) * 0.6 / std::max(1e-9, a.invMass + b.invMass);
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
                b.angularVelocity = b.angularVelocity * std::pow(0.995, h * 60.0);
                b.center += b.velocity * h;
                if (i < objectPhysics.size() && std::abs(objectPhysics[i].orbitOmegaY) > 1e-9) {
                    const ObjectPhysics& p = objectPhysics[i];
                    vec<> rel = b.center - p.orbitCenter;
                    double ang = p.orbitOmegaY * h * M_PI / 180.0;
                    vec<> rel2(std::cos(ang) * rel.x + std::sin(ang) * rel.z, rel.y,
                              -std::sin(ang) * rel.x + std::cos(ang) * rel.z);
                    b.center = p.orbitCenter + rel2;
                }

                double w = safeLen(b.angularVelocity);
                if (w > 1e-8) {
                    b.spinAxis = b.angularVelocity * (1.0 / w);
                    b.spinDeg += (w * h) * 180.0 / M_PI;
                    if (b.spinDeg > 3600.0) b.spinDeg = std::fmod(b.spinDeg, 360.0);
                }

                double minX = -95.0 + b.radius, maxX = 95.0 - b.radius;
                double minZ = -95.0 + b.radius, maxZ = 95.0 - b.radius;
                double minY = b.radius;
                if (b.center.x < minX) { b.center.x = minX; b.velocity.x = std::abs(b.velocity.x); }
                else if (b.center.x > maxX) { b.center.x = maxX; b.velocity.x = -std::abs(b.velocity.x); }
                if (b.center.z < minZ) { b.center.z = minZ; b.velocity.z = std::abs(b.velocity.z); }
                else if (b.center.z > maxZ) { b.center.z = maxZ; b.velocity.z = -std::abs(b.velocity.z); }
                if (b.center.y < minY) {
                    b.center.y = minY;
                    b.velocity.y = std::abs(b.velocity.y) * b.restitution;
                    if (b.useFriction) {
                        const double f = std::clamp(1.0 - b.groundFriction * h, 0.0, 1.0);
                        b.velocity.x *= f;
                        b.velocity.z *= f;
                    }
                    b.angularVelocity += vec<>(0.4, 0, -0.3) * b.invInertia;
                }
            }

            for (int i = 0; i < static_cast<int>(bodies.size()); ++i)
                for (int j = i + 1; j < static_cast<int>(bodies.size()); ++j) {
                    if (bodies[i].groupId >= 0 && bodies[i].groupId == bodies[j].groupId)
                        continue;
                    if (!bodies[i].isLeader && bodies[i].groupId >= 0)
                        continue;
                    if (!bodies[j].isLeader && bodies[j].groupId >= 0)
                        continue;
                    Contact c;
                    bool hit = detectCollision(i, j, c);
                    if (!hit) {
                        // Swept sphere fallback for fast motion to reduce tunneling.
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
                    if (hit)
                        resolveCollision(i, j, c);
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
        double dt = std::clamp(t - physicsTime, 0.0, 1.0 / 30.0);
        physicsTime = t;
        stepPhysics(dt);

        double proj[16], viewMat[16];
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetDoublev(GL_MODELVIEW_MATRIX, viewMat);
        

        double planes[6][4];
        extractFrustumPlanesFromProj(planes, proj);

        // Draw textured objects (ground & sky) without culling
        glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        int drawnCount = 0;   // for debugging
        for (auto obj : texturedObjects) {
            vec<> center;
            double radius;
            obj->getBoundingSphere(center, radius, t);

            vec<> eyeCenter = transformPoint(center, viewMat);

            if (sphereInFrustum(planes, eyeCenter, radius)) {
                glPushMatrix();
                obj->Draw(t);
                glPopMatrix();
                drawnCount++;
            }
        }
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

        for (size_t i = 0; i < Objects.size(); ++i) {
            based* el = Objects[i];
            std::vector<std::pair<vec<>, double>> parts;
            el->collectBoundingSpheres(parts, t);
            if (parts.empty()) {
                vec<> c; double r = 0;
                el->emergency_bounding_sphere_calc_protocol(c, r, t);
                parts.push_back({c, r});
            }

            bool visible = false;
            for (auto pr : parts) {
                vec<> center = pr.first;
                double radius = pr.second;
                if (i < bodies.size()) {
                    const BodyState& b = bodies[i];
                    vec<> d = b.center - b.baseCenter;
                    center = center + d;
                    center = center - b.center;
                    center = rotateAroundAxis(center, b.spinAxis, b.spinDeg * M_PI / 180.0);
                    center = center + b.center;
                }
                vec<> eyeCenter = transformPoint(center, viewMat);
                if (sphereInFrustum(planes, eyeCenter, radius)) {
                    visible = true;
                    break;
                }
            }

            if (visible) {
                glPushMatrix();
                if (i < bodies.size()) {
                    const BodyState& b = bodies[i];
                    vec<> d = b.center - b.baseCenter;
                    glTranslated(d.x, d.y, d.z);
                    glTranslated(b.center.x, b.center.y, b.center.z);
                    glRotated(b.spinDeg, b.spinAxis.x, b.spinAxis.y, b.spinAxis.z);
                    glTranslated(-b.center.x, -b.center.y, -b.center.z);
                }
                el->Draw(t);
                glPopMatrix();
                drawnCount++;
            }
        }

        // std::cout << "drawnCount = " << drawnCount << std::endl;
        lastDrawnCount = drawnCount;

        glPopMatrix();
    }
};
