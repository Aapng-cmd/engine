#include "PreviewWidget.h"

#include "collision_mesh.h"
#include "collision_repr.h"
#include "editable_mesh.h"
#include "engine_power.h"
#include "object_factory.h"
#include "figures.h"
#include "fourd_figure.h"
#include "fourd_math.h"
#include "render_material.h"
#include "manual_shapes.h"
#include "scene.h"
#include "textures_path.h"
#include "transform_wrapper.h"

#include <cmath>
#include "textures.h"

#include <GL/glut.h>
#include <GL/glu.h>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>

static vec<> previewFigureColor(based* el)
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
    if (auto* em = dynamic_cast<EditableMesh*>(el))
        return em->color;
    return vec<>(0.75, 0.75, 0.75);
}

static void unpackMesh(const SceneObject& o, std::vector<vec<>>& verts, std::vector<int>& indices)
{
    verts.clear();
    indices.clear();
    const int nv = o.meshVerts.size() / 3;
    verts.reserve(static_cast<size_t>(nv));
    for (int i = 0; i < nv; ++i)
        verts.push_back(vec<>(o.meshVerts[i * 3], o.meshVerts[i * 3 + 1], o.meshVerts[i * 3 + 2]));
    indices.reserve(static_cast<size_t>(o.meshIndices.size()));
    for (int idx : o.meshIndices)
        indices.push_back(idx);
}

static void packMesh(SceneObject& o, const std::vector<vec<>>& verts, const std::vector<int>& indices)
{
    o.meshVerts.clear();
    o.meshIndices.clear();
    o.meshVerts.reserve(static_cast<int>(verts.size() * 3));
    for (const vec<>& v : verts) {
        o.meshVerts.append(v.x);
        o.meshVerts.append(v.y);
        o.meshVerts.append(v.z);
    }
    o.meshIndices.reserve(static_cast<int>(indices.size()));
    for (int i : indices)
        o.meshIndices.append(i);
}

static void unpackTets(const SceneObject& o, std::vector<Tet4>& tets)
{
    std::vector<double> packed;
    packed.reserve(static_cast<size_t>(o.tetVerts.size()));
    for (double v : o.tetVerts)
        packed.push_back(v);
    unpackFourDTets(packed, tets);
}

static void packTets(SceneObject& o, const std::vector<Tet4>& tets)
{
    std::vector<double> packed;
    packFourDTets(tets, packed);
    o.tetVerts.clear();
    o.tetVerts.reserve(static_cast<int>(packed.size()));
    for (double v : packed)
        o.tetVerts.append(v);
}

static bool isFourDSceneType(const QString& type)
{
    return fourd::isFourDType(type.toStdString());
}

static bool isCameraType(const QString& type)
{
    return type == QLatin1String("camera");
}

static Scene::ObjectPhysics physicsFromSceneObject(const SceneObject& o)
{
    Scene::ObjectPhysics p;
    p.velocity = vec<>(o.vx, o.vy, o.vz);
    p.orbitCenter = vec<>(o.orbitX, o.orbitY, o.orbitZ);
    p.orbitOmegaY = o.orbitOmegaY;
    p.groupId = o.groupId;
    p.gravityMode = o.gravityMode;
    p.useFriction = o.useFriction;
    p.collide = o.collide;
    p.alpha = o.alpha;
    p.massOverride = o.mass;
    p.gravity = vec<>(o.gravityX, o.gravityY, o.gravityZ);
    p.gravTargetPoint = vec<>(o.gravTargetX, o.gravTargetY, o.gravTargetZ);
    p.gravStrength = o.gravStrength;
    p.gravTargetObject = o.gravTargetObject;
    p.collisionSubdiv = o.collisionSubdiv;
    p.groundFriction = o.groundFriction;
    p.restitution = o.restitution;
    p.pk = o.pk;
    p.vk = o.vk;
    p.isStatic = o.isStatic;
    p.rwx = o.rwx;
    p.rwy = o.rwy;
    p.rwz = o.rwz;
    p.scriptPath = o.scriptPath.toStdString();
    if (isCameraType(o.type)) {
        p.collide = 0;
        p.isStatic = 1;
        p.gravityMode = 0;
    }
    return p;
}

PreviewWidget::PreviewWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(33);
    connect(m_playTimer, &QTimer::timeout, this, &PreviewWidget::onPlayTick);
}

PreviewWidget::~PreviewWidget()
{
    makeCurrent();
    clearPlayScene();
    for (based* p : m_objects)
        delete p;
    m_objects.clear();
    delete m_ground;
    m_ground = nullptr;
    delete m_sky;
    m_sky = nullptr;
    doneCurrent();
}

void PreviewWidget::setRepoRoot(const QString& root)
{
    m_repoRoot = root;
    if (!root.isEmpty())
        setInnerDirectoryOverride(QDir(root).filePath(QStringLiteral("inner")).toStdString());
    markSceneDirty();
}

void PreviewWidget::setSceneData(SceneData* data)
{
    m_scene = data;
    markSceneDirty();
}

void PreviewWidget::markSceneDirty()
{
    if (m_playState != PlayState::Stopped)
        stopPlay();
    m_dirty = true;
    update();
}

void PreviewWidget::setSelectedObject(int index)
{
    m_selectedObject = index;
    m_gizmoHover = GizmoPart::None;
    m_gizmoDrag = GizmoPart::None;
    update();
}

void PreviewWidget::cameraEye(double& ex, double& ey, double& ez) const
{
    const double cp = std::cos(m_pitch), sp = std::sin(m_pitch);
    const double cy = std::cos(m_yaw), sy = std::sin(m_yaw);
    ex = m_targetX + m_dist * cp * sy;
    ey = m_targetY + m_dist * sp;
    ez = m_targetZ + m_dist * cp * cy;
}

void PreviewWidget::initializeGL()
{
    if (!m_glutInited) {
        int argc = 1;
        char arg0[] = "preview";
        char* argv[] = {arg0, nullptr};
        glutInit(&argc, argv);
        m_glutInited = true;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    initMatteSceneLighting();
}

void PreviewWidget::resizeGL(int w, int h)
{
    if (h <= 0)
        h = 1;
    glViewport(0, 0, w, h);
}

static QString resolveTexturePath(const QString& repo, const QString& rel)
{
    if (QFileInfo(rel).isAbsolute())
        return rel;
    return QDir(repo).filePath(rel);
}

void PreviewWidget::rebuildObjectsAndTextures()
{
    for (based* p : m_objects)
        delete p;
    m_objects.clear();
    delete m_ground;
    m_ground = nullptr;
    delete m_sky;
    m_sky = nullptr;
    m_texIds.clear();
    if (!m_scene)
        return;

    auto envTex = [&](const QString& rel) -> GLuint {
        const QString path = resolveTexturePath(m_repoRoot, rel);
        return LoadTexID(path.toStdString());
    };
    m_ground = new GroundPlane(envTex(m_scene->env.groundTexture), m_scene->env.groundEdge1, m_scene->env.groundEdge2);
    m_sky = new SkySphere(envTex(m_scene->env.skyTexture), m_scene->env.skyRadius);

    QHash<QString, GLuint> texByPath;
    for (const QString& tp : m_scene->textures) {
        const QString path = resolveTexturePath(m_repoRoot, tp);
        if (!texByPath.contains(path))
            texByPath.insert(path, LoadTexID(path.toStdString()));
        m_texIds.push_back(texByPath.value(path));
    }

    for (const SceneObject& o : m_scene->objects) {
        std::vector<double> ex;
        for (double v : o.extra)
            ex.push_back(v);
        GLuint tex = 0;
        if (o.texIndex >= 0 && o.texIndex < m_scene->textures.size()) {
            const QString path = resolveTexturePath(m_repoRoot, m_scene->textures[o.texIndex]);
            tex = texByPath.value(path, 0);
        }

        std::string err;
        based* obj = createSceneObject(o.type.toStdString(), o.px, o.py, o.pz, o.sx, o.sy, o.sz, o.rx, o.ry, o.rz, ex,
                                       tex, &err);
        if (!obj) {
            // Keep list index aligned with m_scene->objects for picking/selection.
            obj = new EditorSphere(vec<>(o.px, o.py, o.pz), vec<>(o.sx, o.sy, o.sz), o.rx, o.ry, o.rz, 1.0,
                                   vec<>(0.5, 0.2, 0.2), 0);
        }
        setFigureRenderAlpha(obj, o.alpha);
        applyObjectExtras(obj, o);
        applyFigureColor(obj, vec<>(o.cr, o.cg, o.cb));
        m_objects.push_back(obj);
    }
}

static bool raySphere(vec<> orig, vec<> dir, vec<> center, double R, double& tHit)
{
    vec<> oc = orig - center;
    const double b = oc.dot(dir);
    const double c = oc.len2() - R * R;
    const double disc = b * b - c;
    if (disc < 0)
        return false;
    double s = std::sqrt(disc);
    double t0 = -b - s;
    double t1 = -b + s;
    tHit = (t0 > 1e-3) ? t0 : ((t1 > 1e-3) ? t1 : -1);
    return tHit > 1e-3;
}

void PreviewWidget::pickAt(int x, int y)
{
    makeCurrent();
    GLint vp[4] = {0, 0, width(), height()};
    glGetIntegerv(GL_VIEWPORT, vp);

    GLdouble model[16], proj[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);

    double winY = double(vp[3] - y);
    GLdouble nearX, nearY, nearZ, farX, farY, farZ;
    gluUnProject(x, winY, 0.0, model, proj, vp, &nearX, &nearY, &nearZ);
    gluUnProject(x, winY, 1.0, model, proj, vp, &farX, &farY, &farZ);

    vec<> orig(nearX, nearY, nearZ);
    vec<> dir(farX - nearX, farY - nearY, farZ - nearZ);
    const double dl2 = dir.len2();
    if (dl2 < 1e-18)
        return;
    const double dl = std::sqrt(dl2);
    dir = dir * (1.0 / dl);

    int best = -1;
    double bestT = std::numeric_limits<double>::infinity();
    for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
        if (m_scene && i < m_scene->objects.size() && isCameraType(m_scene->objects[i].type))
            continue;
        vec<> c;
        double r = 0;
        m_objects[static_cast<size_t>(i)]->emergency_bounding_sphere_calc_protocol(c, r, 0);
        double t;
        if (raySphere(orig, dir, c, r, t) && t < bestT) {
            bestT = t;
            best = i;
        }
    }
    emit objectPicked(best);
}

void PreviewWidget::paintGL()
{
    if (m_dirty && m_playState == PlayState::Stopped) {
        rebuildObjectsAndTextures();
        m_dirty = false;
    }

    glClearColor(0.12f, 0.14f, 0.18f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = height() > 0 ? double(width()) / double(height()) : 1.0;
    gluPerspective(50.0, aspect, 0.5, 8000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    double ex, ey, ez;
    cameraEye(ex, ey, ez);
    gluLookAt(ex, ey, ez, m_targetX, m_targetY, m_targetZ, 0, 1, 0);

    GLfloat lp[] = {static_cast<GLfloat>(ex + 50), static_cast<GLfloat>(ey + 80), static_cast<GLfloat>(ez + 30), 1.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    initMatteSceneLighting();

    glDisable(GL_LIGHTING);
    glLineWidth(2.f);
    auto axis = [&](float r, float g, float b, double x0, double y0, double z0, double x1, double y1, double z1) {
        glColor3f(r, g, b);
        glBegin(GL_LINES);
        glVertex3d(x0, y0, z0);
        glVertex3d(x1, y1, z1);
        glEnd();
    };
    axis(1, 0.2f, 0.2f, 0, 0, 0, 6, 0, 0);
    axis(0.2f, 1, 0.2f, 0, 0, 0, 0, 6, 0);
    axis(0.2f, 0.2f, 1, 0, 0, 0, 0, 0, 6);
    glEnable(GL_LIGHTING);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    if (m_sky)
        m_sky->Draw(0);
    glEnable(GL_LIGHTING);
    const bool playing = (m_playState != PlayState::Stopped && m_playScene);
    if (playing) {
        m_playScene->updateSceneLight(vec<>(ex, ey, ez));
        for (size_t i = 0; i < m_playScene->Objects.size(); ++i) {
            if (m_scene && static_cast<int>(i) < m_scene->objects.size() &&
                isCameraType(m_scene->objects[static_cast<int>(i)].type))
                continue;
            based* el = m_playScene->Objects[i];
            double a = 1.0;
            if (i < m_playScene->bodies.size())
                a = m_playScene->bodies[i].alpha;
            setFigureRenderAlpha(el, a);
            const AlphaReflect ar = decomposeAlphaReflect(a);
            const vec<> tint = previewFigureColor(el);
            const vec<>* tintPtr = (el->textureID == 0) ? &tint : nullptr;
            glPushAttrib(GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
            applyFigureMaterial(ar.opacity, ar.reflect, tintPtr);
            applySurfacePassState(ar.opacity, ar.reflect);
            if (i < m_playScene->bodies.size())
                Scene::drawObjectRigidBody(el, m_playScene->bodies[i], 0);
            else
                el->Draw(0);
            glPopAttrib();
        }
    } else
    for (size_t i = 0; i < m_objects.size(); ++i) {
        if (m_scene && static_cast<int>(i) < m_scene->objects.size() &&
            isCameraType(m_scene->objects[static_cast<int>(i)].type))
            continue;
        double a = 1.0;
        if (m_scene && static_cast<int>(i) < m_scene->objects.size())
            a = m_scene->objects[static_cast<int>(i)].alpha;
        setFigureRenderAlpha(m_objects[i], a);
        const AlphaReflect ar = decomposeAlphaReflect(a);
        const vec<> tint = previewFigureColor(m_objects[i]);
        const vec<>* tintPtr = (m_objects[i]->textureID == 0) ? &tint : nullptr;
        glPushAttrib(GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
        applyFigureMaterial(ar.opacity, ar.reflect, tintPtr);
        applySurfacePassState(ar.opacity, ar.reflect);
        const bool transparent = ar.opacity < 0.999;
        if (transparent)
            glDepthMask(GL_FALSE);
        double kW = 0.0;
        if (m_scene && static_cast<int>(i) < m_scene->objects.size())
            kW = m_scene->objects[static_cast<int>(i)].pk;
        if (auto* f4 = dynamic_cast<FourDWireFigure*>(m_objects[i]))
            f4->drawSliced(m_cam4dK, kW);
        else
            m_objects[i]->Draw(0);
        if (transparent)
            glDepthMask(GL_TRUE);
        glPopAttrib();
    }

    if (!playing && m_selectedObject >= 0 && m_selectedObject < static_cast<int>(m_objects.size()) &&
        !(m_scene && m_selectedObject < m_scene->objects.size() &&
          isCameraType(m_scene->objects[m_selectedObject].type))) {
        based* sel = m_objects[static_cast<size_t>(m_selectedObject)];
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glLineWidth(2.5f);
        glColor3f(0.2f, 0.55f, 1.0f);

        if (usesTriangleCollision(collisionReprForObject(sel))) {
            /* Mirror the slider, not a camera LOD, so the overlay shows the mesh physics will use. */
            int subdiv = 4;
            if (m_scene && m_selectedObject < m_scene->objects.size())
                subdiv = std::clamp(m_scene->objects[m_selectedObject].collisionSubdiv, 1, engine::maxCollisionSubdiv());
            const int groupId = (m_scene && m_selectedObject < m_scene->objects.size())
                                    ? m_scene->objects[m_selectedObject].groupId
                                    : -1;
            auto drawMesh = [&](based* obj) {
                std::vector<CollTri> tris;
                collision::buildObjectCollisionMesh(obj, tris, subdiv);
                for (const CollTri& tri : tris) {
                    glBegin(GL_LINE_LOOP);
                    glVertex3d(tri.v0.x, tri.v0.y, tri.v0.z);
                    glVertex3d(tri.v1.x, tri.v1.y, tri.v1.z);
                    glVertex3d(tri.v2.x, tri.v2.y, tri.v2.z);
                    glEnd();
                }
            };
            drawMesh(sel);
            /* A merged figure shares one slider: show every part at the same density. */
            if (groupId >= 0 && m_scene) {
                for (int gi = 0; gi < m_scene->objects.size(); ++gi) {
                    if (gi == m_selectedObject || m_scene->objects[gi].groupId != groupId)
                        continue;
                    if (gi < static_cast<int>(m_objects.size()) && m_objects[static_cast<size_t>(gi)])
                        drawMesh(m_objects[static_cast<size_t>(gi)]);
                }
            }
        } else {
            std::vector<std::pair<vec<>, double>> parts;
            sel->getBoundingSpheres(parts, 0);
            if (parts.empty()) {
                vec<> c;
                double r = 0;
                sel->emergency_bounding_sphere_calc_protocol(c, r, 0);
                parts.push_back({c, r});
            }
            for (const auto& pr : parts) {
                glPushMatrix();
                glTranslated(pr.first.x, pr.first.y, pr.first.z);
                glutWireSphere(std::max(0.001, pr.second), 14, 10);
                glPopMatrix();
            }
        }

        if (m_scene && m_selectedObject < m_scene->objects.size()) {
            const SceneObject& so = m_scene->objects[m_selectedObject];
            const vec<> anchor(so.px, so.py, so.pz);
            vec<> v(so.vx, so.vy, so.vz);
            if (v.len2() > 1e-10) {
                vec<> tip = anchor + v;
                glColor3f(0.95f, 0.35f, 0.1f);
                glBegin(GL_LINES);
                glVertex3d(anchor.x, anchor.y, anchor.z);
                glVertex3d(tip.x, tip.y, tip.z);
                glEnd();
            }
            vec<> orbit(so.orbitX, so.orbitY, so.orbitZ);
            glColor3f(1.0f, 0.95f, 0.1f);
            glPushMatrix();
            glTranslated(orbit.x, orbit.y, orbit.z);
            glutWireSphere(0.12, 10, 8);
            glPopMatrix();
            glBegin(GL_LINES);
            glVertex3d(anchor.x, anchor.y, anchor.z);
            glVertex3d(orbit.x, orbit.y, orbit.z);
            glEnd();
        }
        glPopAttrib();
    }

    if (m_editMode && m_editVert >= 0) {
        if (EditableMesh* mesh = dynamic_cast<EditableMesh*>(selectedMeshObject())) {
            if (m_editVert < mesh->vertCount()) {
                const vec<> w = mesh->worldVertex(m_editVert);
                glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
                glDisable(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glColor3f(1.f, 0.85f, 0.1f);
                glPushMatrix();
                glTranslated(w.x, w.y, w.z);
                glutSolidSphere(0.08, 10, 8);
                glPopMatrix();
                glPopAttrib();
            }
        } else if (FourDWireFigure* f4 = asFourDFigure(selectedMeshObject())) {
            std::vector<Vec4> uniq;
            f4->uniqueLocalVerts(uniq);
            if (m_editVert < static_cast<int>(uniq.size())) {
                const double kOff = (m_scene && m_selectedObject >= 0 && m_selectedObject < m_scene->objects.size())
                                        ? m_scene->objects[m_selectedObject].pk
                                        : f4->kPos;
                const Vec4 w = f4->worldVert(uniq[static_cast<size_t>(m_editVert)], f4->pos, kOff);
                glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
                glDisable(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glColor3f(1.f, 0.55f, 0.95f);
                glPushMatrix();
                glTranslated(w.x, w.y, w.z);
                glutSolidSphere(0.1, 10, 8);
                glPopMatrix();
                glPopAttrib();
            }
        }
    }

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    if (m_ground)
        m_ground->Draw(0);
    glEnable(GL_LIGHTING);

    drawDebugOverlays();
    if (!playing && !m_editMode)
        drawGizmos();
}

void PreviewWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Tab) {
        if (m_scene && m_selectedObject >= 0 && m_selectedObject < m_scene->objects.size()) {
            const QString& tp = m_scene->objects[m_selectedObject].type;
            if (tp == QLatin1String("mesh") || isFourDSceneType(tp))
                setEditMode(!m_editMode);
        }
        e->accept();
        return;
    }
    if (m_editMode) {
        if (e->key() == Qt::Key_G) {
            m_grabbing = true;
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_E) {
            if (dynamic_cast<EditableMesh*>(selectedMeshObject()))
                meshExtrudeSelected();
            e->accept();
            return;
        }
    }
    if (e->key() == Qt::Key_Semicolon) {
        setDebugLayer((m_debugLayer + 1) % 3);
        e->accept();
        return;
    }
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Plus || e->key() == Qt::Key_Equal) {
            m_dist *= 0.9;
            if (m_dist < 2)
                m_dist = 2;
            emitCameraIfNeeded();
            update();
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_Minus || e->key() == Qt::Key_Underscore) {
            m_dist /= 0.9;
            if (m_dist > 500)
                m_dist = 500;
            emitCameraIfNeeded();
            update();
            e->accept();
            return;
        }
    }
    QOpenGLWidget::keyPressEvent(e);
}

void PreviewWidget::mousePressEvent(QMouseEvent* e)
{
    setFocus(Qt::MouseFocusReason);
    m_lastPos = e->pos();
    m_pressPos = e->pos();
    m_gizmoDrag = GizmoPart::None;
    if (e->button() == Qt::LeftButton) {
        if (!m_editMode && m_playState != PlayState::Playing) {
            const GizmoPart hit = hitGizmo(e->pos().x(), e->pos().y());
            if (hit != GizmoPart::None && m_scene && m_selectedObject >= 0 &&
                m_selectedObject < m_scene->objects.size()) {
                m_gizmoDrag = hit;
                const SceneObject& o = m_scene->objects[m_selectedObject];
                m_dragStartPos = vec<>(o.px, o.py, o.pz);
                m_dragStartRx = o.rx;
                m_dragStartRy = o.ry;
                m_dragStartRz = o.rz;
                m_dragStartYaw = m_yaw;
                m_dragStartPitch = m_pitch;
                m_dragStartDist = m_dist;
                vec<> orig, dir;
                screenRay(e->pos().x(), e->pos().y(), orig, dir);
                vec<> axis(0, 0, 1);
                if (hit == GizmoPart::AxisX || hit == GizmoPart::RotX)
                    axis = vec<>(1, 0, 0);
                else if (hit == GizmoPart::AxisY || hit == GizmoPart::RotY)
                    axis = vec<>(0, 1, 0);
                if (hit == GizmoPart::AxisX || hit == GizmoPart::AxisY || hit == GizmoPart::AxisZ) {
                    double t = 0;
                    const vec<> w0 = orig - m_dragStartPos;
                    const double b = dir.dot(axis);
                    const double d = dir.dot(w0);
                    const double ee = axis.dot(w0);
                    const double denom = 1.0 - b * b;
                    if (std::abs(denom) > 1e-12)
                        t = (ee - b * d) / denom;
                    m_dragAxisT0 = t;
                } else {
                    const double dn = dir.dot(axis);
                    if (std::abs(dn) > 1e-9) {
                        const double tt = (m_dragStartPos - orig).dot(axis) / dn;
                        if (tt > 1e-4) {
                            const vec<> hitP = orig + dir * tt;
                            vec<> tmp = std::abs(axis.y) < 0.9 ? vec<>(0, 1, 0) : vec<>(1, 0, 0);
                            vec<> u = tmp ^ axis;
                            const double ul = u.len();
                            if (ul > 1e-9)
                                u = u * (1.0 / ul);
                            const vec<> v = axis ^ u;
                            const vec<> dlt = hitP - m_dragStartPos;
                            m_dragStartAngle = std::atan2(dlt.dot(v), dlt.dot(u));
                        }
                    }
                }
                m_rotating = false;
                return;
            }
        }
        m_rotating = !m_editMode;
    } else if (e->button() == Qt::RightButton)
        m_panning = true;
}

void PreviewWidget::mouseMoveEvent(QMouseEvent* e)
{
    QPoint d = e->pos() - m_lastPos;
    m_lastPos = e->pos();
    if (m_editMode && m_grabbing && m_editVert >= 0) {
        vec<> right, up, fwd;
        cameraBasis(right, up, fwd);
        const double s = m_dist * 0.0025;
        if (EditableMesh* mesh = dynamic_cast<EditableMesh*>(selectedMeshObject())) {
            if (m_editVert < mesh->vertCount()) {
                vec<> p = mesh->verts[static_cast<size_t>(m_editVert)];
                p = p + right * (d.x() * s) + up * (-d.y() * s);
                mesh->setVertex(m_editVert, p);
                syncSelectedMeshToScene();
                update();
            }
        } else if (FourDWireFigure* f4 = asFourDFigure(selectedMeshObject())) {
            std::vector<Vec4> uniq;
            f4->uniqueLocalVerts(uniq);
            if (m_editVert < static_cast<int>(uniq.size()) && m_scene &&
                m_selectedObject >= 0 && m_selectedObject < m_scene->objects.size()) {
                const SceneObject& so = m_scene->objects[m_selectedObject];
                const Vec4 w = f4->worldVert(uniq[static_cast<size_t>(m_editVert)], f4->pos, so.pk);
                vec<> xyz(w.x, w.y, w.z);
                xyz = xyz + right * (d.x() * s) + up * (-d.y() * s);
                if (f4->moveUniqueVertOnSlice(m_editVert, xyz, m_cam4dK, f4->pos, so.pk))
                    syncSelectedTetsToScene();
                update();
            }
        }
        return;
    }
    if (m_gizmoDrag != GizmoPart::None && m_scene && m_selectedObject >= 0 &&
        m_selectedObject < m_scene->objects.size()) {
        SceneObject& o = m_scene->objects[m_selectedObject];
        vec<> orig, dir;
        screenRay(e->pos().x(), e->pos().y(), orig, dir);
        vec<> axis(0, 0, 1);
        if (m_gizmoDrag == GizmoPart::AxisX || m_gizmoDrag == GizmoPart::RotX)
            axis = vec<>(1, 0, 0);
        else if (m_gizmoDrag == GizmoPart::AxisY || m_gizmoDrag == GizmoPart::RotY)
            axis = vec<>(0, 1, 0);
        if (m_gizmoDrag == GizmoPart::AxisX || m_gizmoDrag == GizmoPart::AxisY || m_gizmoDrag == GizmoPart::AxisZ) {
            const vec<> w0 = orig - m_dragStartPos;
            const double b = dir.dot(axis);
            const double dd = dir.dot(w0);
            const double ee = axis.dot(w0);
            const double denom = 1.0 - b * b;
            double t = m_dragAxisT0;
            if (std::abs(denom) > 1e-12)
                t = (ee - b * dd) / denom;
            const vec<> np = m_dragStartPos + axis * (t - m_dragAxisT0);
            o.px = np.x;
            o.py = np.y;
            o.pz = np.z;
            if (selectedIsCamera()) {
                m_targetX = o.px;
                m_targetY = o.py;
                m_targetZ = o.pz;
            } else
                applyLiveObjectTransform(m_selectedObject);
        } else {
            const double dn = dir.dot(axis);
            if (std::abs(dn) > 1e-9) {
                const double tt = (m_dragStartPos - orig).dot(axis) / dn;
                if (tt > 1e-4) {
                    const vec<> hitP = orig + dir * tt;
                    vec<> tmp = std::abs(axis.y) < 0.9 ? vec<>(0, 1, 0) : vec<>(1, 0, 0);
                    vec<> u = tmp ^ axis;
                    const double ul = u.len();
                    if (ul > 1e-9)
                        u = u * (1.0 / ul);
                    const vec<> v = axis ^ u;
                    const vec<> dlt = hitP - m_dragStartPos;
                    const double ang = std::atan2(dlt.dot(v), dlt.dot(u));
                    const double dAng = ang - m_dragStartAngle;
                    const double deg = dAng * 180.0 / M_PI;
                    if (selectedIsCamera()) {
                        if (m_gizmoDrag == GizmoPart::RotX)
                            m_pitch = std::clamp(m_dragStartPitch + dAng, -1.4, 1.4);
                        else if (m_gizmoDrag == GizmoPart::RotY)
                            m_yaw = m_dragStartYaw + dAng;
                        writeOrbitToObject(o);
                    } else {
                        o.rx = m_dragStartRx;
                        o.ry = m_dragStartRy;
                        o.rz = m_dragStartRz;
                        if (m_gizmoDrag == GizmoPart::RotX)
                            o.rx += deg;
                        else if (m_gizmoDrag == GizmoPart::RotY)
                            o.ry += deg;
                        else
                            o.rz += deg;
                        applyLiveObjectTransform(m_selectedObject);
                    }
                }
            }
        }
        emit transformEdited(m_selectedObject);
        update();
        return;
    }
    if (m_gizmoDrag == GizmoPart::None && !m_rotating && !m_panning && !m_editMode) {
        const GizmoPart h = hitGizmo(e->pos().x(), e->pos().y());
        if (h != m_gizmoHover) {
            m_gizmoHover = h;
            update();
        }
    }
    if (m_rotating) {
        m_yaw -= d.x() * 0.01;
        m_pitch += d.y() * 0.01;
        const double lim = 1.4;
        if (m_pitch > lim)
            m_pitch = lim;
        if (m_pitch < -lim)
            m_pitch = -lim;
        emitCameraIfNeeded();
        update();
    } else if (m_panning) {
        double ex, ey, ez;
        cameraEye(ex, ey, ez);
        vec<> F(m_targetX - ex, m_targetY - ey, m_targetZ - ez);
        const double fl2 = F.len2();
        if (fl2 < 1e-18)
            return;
        const double fl = std::sqrt(fl2);
        F = F * (1.0 / fl);
        vec<> Wup(0, 1, 0);
        vec<> Rv = Wup ^ F;
        const double rl2 = Rv.len2();
        if (rl2 < 1e-18)
            return;
        const double rl = std::sqrt(rl2);
        Rv = Rv * (1.0 / rl);
        vec<> Uv = F ^ Rv;
        double pan = 0.06;
        m_targetX += (-Rv.x * d.x() + Uv.x * d.y()) * pan;
        m_targetY += (-Rv.y * d.x() + Uv.y * d.y()) * pan;
        m_targetZ += (-Rv.z * d.x() + Uv.z * d.y()) * pan;
        emitCameraIfNeeded();
        update();
    }
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        const bool wasGizmo = m_gizmoDrag != GizmoPart::None;
        if (!wasGizmo && (e->pos() - m_pressPos).manhattanLength() < 5) {
            if (m_editMode)
                pickEditAt(e->pos().x(), e->pos().y());
            else
                pickAt(e->pos().x(), e->pos().y());
        }
        if (m_rotating)
            emitCameraIfNeeded();
        m_rotating = false;
        m_gizmoDrag = GizmoPart::None;
    }
    if (e->button() == Qt::RightButton) {
        if (m_panning)
            emitCameraIfNeeded();
        m_panning = false;
    }
}

void PreviewWidget::wheelEvent(QWheelEvent* e)
{
    double steps = e->angleDelta().y() / 120.0;
    m_dist *= std::pow(0.92, steps);
    if (m_dist < 2)
        m_dist = 2;
    if (m_dist > 500)
        m_dist = 500;
    emitCameraIfNeeded();
    update();
}

void PreviewWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_G)
        m_grabbing = false;
    QOpenGLWidget::keyReleaseEvent(e);
}

void PreviewWidget::applyObjectExtras(based* obj, const SceneObject& o)
{
    if (!obj)
        return;
    applyFourDAngles(obj, o.rwx, o.rwy, o.rwz);
    if (auto* f4 = asFourDFigure(obj))
        f4->kPos = o.pk;
    std::vector<vec<>> verts;
    std::vector<int> inds;
    unpackMesh(o, verts, inds);
    if (!verts.empty())
        applyEditableMeshData(obj, verts, inds);
    std::vector<Tet4> tets;
    unpackTets(o, tets);
    if (!tets.empty())
        applyFourDTets(obj, tets);
}

void PreviewWidget::cameraBasis(vec<>& right, vec<>& up, vec<>& forward) const
{
    double ex, ey, ez;
    cameraEye(ex, ey, ez);
    forward = vec<>(m_targetX - ex, m_targetY - ey, m_targetZ - ez);
    const double fl = forward.len();
    if (fl > 1e-9)
        forward = forward * (1.0 / fl);
    right = vec<>(0, 1, 0) ^ forward;
    const double rl = right.len();
    if (rl > 1e-9)
        right = right * (1.0 / rl);
    else
        right = vec<>(1, 0, 0);
    up = forward ^ right;
}

void PreviewWidget::clearPlayScene()
{
    if (!m_playScene)
        return;
    for (based* p : m_playScene->Objects)
        delete p;
    m_playScene->Objects.clear();
    m_playScene->objectPhysics.clear();
    m_playScene->clearEnvironment();
    m_playScene->objectScripts.clear();
    delete m_playScene;
    m_playScene = nullptr;
}

void PreviewWidget::buildPlayScene()
{
    clearPlayScene();
    if (!m_scene)
        return;
    m_playScene = new Scene();
    m_playScene->editorTexturePaths.clear();
    m_playScene->editorTextureGlIds.clear();
    for (const QString& tp : m_scene->textures) {
        const QString path = resolveTexturePath(m_repoRoot, tp);
        m_playScene->editorTexturePaths.push_back(path.toStdString());
        m_playScene->editorTextureGlIds.push_back(LoadTexID(path.toStdString()));
    }
    int i = 0;
    for (const SceneObject& o : m_scene->objects) {
        std::vector<double> ex;
        for (double v : o.extra)
            ex.push_back(v);
        GLuint tex = 0;
        if (o.texIndex >= 0 && o.texIndex < static_cast<int>(m_playScene->editorTextureGlIds.size()))
            tex = m_playScene->editorTextureGlIds[static_cast<size_t>(o.texIndex)];
        std::string err;
        based* obj = createSceneObject(o.type.toStdString(), o.px, o.py, o.pz, o.sx, o.sy, o.sz, o.rx, o.ry, o.rz, ex,
                                       tex, &err);
        if (!obj)
            obj = new EditorSphere(vec<>(o.px, o.py, o.pz), vec<>(o.sx, o.sy, o.sz), o.rx, o.ry, o.rz, 1.0,
                                   vec<>(0.5, 0.2, 0.2), 0);
        applyObjectExtras(obj, o);
        applyFigureColor(obj, vec<>(o.cr, o.cg, o.cb));
        setFigureRenderAlpha(obj, o.alpha);
        m_playScene->addLoadedObject(obj, physicsFromSceneObject(o));
        m_playScene->objectTextureIndices.push_back(o.texIndex);
        ++i;
    }
    (void)i;
    const QString gpath = resolveTexturePath(m_repoRoot, m_scene->env.groundTexture);
    const QString spath = resolveTexturePath(m_repoRoot, m_scene->env.skyTexture);
    m_playScene->setEnvironment(gpath.toStdString(), m_scene->env.groundEdge1, m_scene->env.groundEdge2,
                                spath.toStdString(), m_scene->env.skyRadius);
    m_playScene->rebuildBodies();
}

void PreviewWidget::startPlay()
{
    if (m_playState == PlayState::Playing)
        return;
    makeCurrent();
    if (m_playState == PlayState::Stopped)
        buildPlayScene();
    m_playState = PlayState::Playing;
    m_playTimer->start();
    emit playStateChanged();
    update();
}

void PreviewWidget::pausePlay()
{
    if (m_playState != PlayState::Playing)
        return;
    m_playTimer->stop();
    m_playState = PlayState::Paused;
    emit playStateChanged();
    update();
}

void PreviewWidget::stopPlay()
{
    m_playTimer->stop();
    makeCurrent();
    clearPlayScene();
    m_playState = PlayState::Stopped;
    m_dirty = true;
    emit playStateChanged();
    update();
}

void PreviewWidget::onPlayTick()
{
    if (!m_playScene || m_playState != PlayState::Playing)
        return;
    double ex, ey, ez;
    cameraEye(ex, ey, ez);
    m_playScene->physicsCameraPos = vec<>(ex, ey, ez);
    m_playScene->physicsTime += 1.0 / 30.0;
    m_playScene->stepPhysics(1.0 / 30.0);
    update();
}

void PreviewWidget::setSliceMode(bool slice)
{
    m_sliceMode = slice;
    update();
}

void PreviewWidget::setCam4dK(double k)
{
    m_cam4dK = k;
    update();
}

void PreviewWidget::setEditMode(bool on)
{
    m_editMode = on;
    m_grabbing = false;
    if (!on) {
        m_editVert = -1;
        m_editFace = -1;
    }
    update();
}

void PreviewWidget::setDebugLayer(int layer)
{
    const int v = ((layer % 3) + 3) % 3;
    if (m_debugLayer == v)
        return;
    m_debugLayer = v;
    emit debugLayerChanged(m_debugLayer);
    update();
}

void PreviewWidget::drawCollisionWire(based* obj, int subdiv, float r, float g, float b)
{
    if (!obj)
        return;
    glColor3f(r, g, b);
    if (usesTriangleCollision(collisionReprForObject(obj))) {
        std::vector<CollTri> tris;
        collision::buildObjectCollisionMesh(obj, tris, std::max(1, subdiv));
        for (const CollTri& tri : tris) {
            glBegin(GL_LINE_LOOP);
            glVertex3d(tri.v0.x, tri.v0.y, tri.v0.z);
            glVertex3d(tri.v1.x, tri.v1.y, tri.v1.z);
            glVertex3d(tri.v2.x, tri.v2.y, tri.v2.z);
            glEnd();
        }
        return;
    }
    std::vector<std::pair<vec<>, double>> parts;
    obj->getBoundingSpheres(parts, 0);
    if (parts.empty()) {
        vec<> c;
        double rad = 0;
        obj->emergency_bounding_sphere_calc_protocol(c, rad, 0);
        parts.push_back({c, rad});
    }
    for (const auto& pr : parts) {
        glPushMatrix();
        glTranslated(pr.first.x, pr.first.y, pr.first.z);
        glutWireSphere(std::max(0.05, pr.second), 12, 10);
        glPopMatrix();
    }
}

void PreviewWidget::drawEditTimeDynamics()
{
    if (!m_scene)
        return;
    glColor3f(1.0f, 0.85f, 0.1f);
    const double horizon = 5.0;
    for (int i = 0; i < m_scene->objects.size(); ++i) {
        const SceneObject& so = m_scene->objects[i];
        if (so.groupId >= 0) {
            bool leader = true;
            for (int j = 0; j < i; ++j) {
                if (m_scene->objects[j].groupId == so.groupId) {
                    leader = false;
                    break;
                }
            }
            if (!leader)
                continue;
        }
        const vec<> c(so.px, so.py, so.pz);
        glColor3f(1.0f, 0.85f, 0.1f);
        glPushMatrix();
        glTranslated(c.x, c.y, c.z);
        glutWireSphere(0.12, 10, 8);
        glPopMatrix();
        vec<> v(so.vx, so.vy, so.vz);
        if (v.len2() > 1e-8) {
            glColor3f(0.95f, 0.35f, 0.1f);
            const vec<> tip = c + v;
            glBegin(GL_LINES);
            glVertex3d(c.x, c.y, c.z);
            glVertex3d(tip.x, tip.y, tip.z);
            glEnd();
            glColor3f(0.2f, 0.75f, 1.0f);
            const vec<> end = c + v * horizon;
            glBegin(GL_LINES);
            glVertex3d(c.x, c.y, c.z);
            glVertex3d(end.x, end.y, end.z);
            glEnd();
        }
    }
}

void PreviewWidget::drawDebugOverlays()
{
    if (m_debugLayer <= 0)
        return;
    const bool playing = (m_playState != PlayState::Stopped && m_playScene);
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.5f);
    if (playing) {
        m_playScene->debugLayer = m_debugLayer;
        if (m_debugLayer == 1)
            m_playScene->drawDebugBoundingSpheres(0);
        else if (m_debugLayer == 2)
            m_playScene->drawDebugDynamics(0);
        glPopAttrib();
        return;
    }
    if (m_debugLayer == 1) {
        glDepthMask(GL_FALSE);
        for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
            int subdiv = 4;
            if (m_scene && i < m_scene->objects.size())
                subdiv = std::clamp(m_scene->objects[i].collisionSubdiv, 1, engine::maxCollisionSubdiv());
            drawCollisionWire(m_objects[static_cast<size_t>(i)], subdiv, 0.2f, 0.95f, 0.35f);
        }
        glDepthMask(GL_TRUE);
    } else if (m_debugLayer == 2) {
        drawEditTimeDynamics();
    }
    glPopAttrib();
}

based* PreviewWidget::selectedMeshObject() const
{
    if (m_selectedObject < 0 || m_selectedObject >= static_cast<int>(m_objects.size()))
        return nullptr;
    return m_objects[static_cast<size_t>(m_selectedObject)];
}

void PreviewWidget::syncSelectedMeshToScene()
{
    if (!m_scene || m_selectedObject < 0 || m_selectedObject >= m_scene->objects.size())
        return;
    std::vector<vec<>> verts;
    std::vector<int> inds;
    copyEditableMeshData(selectedMeshObject(), verts, inds);
    packMesh(m_scene->objects[m_selectedObject], verts, inds);
    emit meshEdited(m_selectedObject);
}

void PreviewWidget::syncSelectedTetsToScene()
{
    if (!m_scene || m_selectedObject < 0 || m_selectedObject >= m_scene->objects.size())
        return;
    std::vector<Tet4> tets;
    copyFourDTets(selectedMeshObject(), tets);
    packTets(m_scene->objects[m_selectedObject], tets);
    emit meshEdited(m_selectedObject);
}

bool PreviewWidget::convertSelectedToMesh()
{
    if (m_playState != PlayState::Stopped)
        stopPlay();
    if (!m_scene || m_selectedObject < 0 || m_selectedObject >= m_scene->objects.size())
        return false;
    SceneObject& o = m_scene->objects[m_selectedObject];
    if (o.type == QLatin1String("mesh"))
        return true;
    based* obj = selectedMeshObject();
    if (!obj)
        return false;
    std::vector<CollTri> tris;
    const int subdiv = std::clamp(o.collisionSubdiv, 1, 4);
    if (!collision::buildObjectCollisionMesh(obj, tris, subdiv) || tris.empty())
        return false;
    EditableMesh tmp(vec<>(o.px, o.py, o.pz), vec<>(o.sx, o.sy, o.sz), o.rx, o.ry, o.rz);
    if (!tmp.fillFromWorldTris(tris))
        return false;
    o.type = QLatin1String("mesh");
    o.extra.clear();
    o.tetVerts.clear();
    packMesh(o, tmp.verts, tmp.indices);
    m_dirty = true;
    emit meshEdited(m_selectedObject);
    update();
    return true;
}

void PreviewWidget::meshAddCube()
{
    EditableMesh* mesh = dynamic_cast<EditableMesh*>(selectedMeshObject());
    if (!mesh)
        return;
    mesh->fillUnitCube();
    syncSelectedMeshToScene();
    update();
}

void PreviewWidget::meshAddPlane()
{
    EditableMesh* mesh = dynamic_cast<EditableMesh*>(selectedMeshObject());
    if (!mesh)
        return;
    mesh->fillPlane(1.0, 1.0);
    syncSelectedMeshToScene();
    update();
}

void PreviewWidget::meshExtrudeSelected()
{
    EditableMesh* mesh = dynamic_cast<EditableMesh*>(selectedMeshObject());
    if (!mesh)
        return;
    const int face = m_editFace >= 0 ? m_editFace : 0;
    if (mesh->extrudeFace(face, 0.35)) {
        syncSelectedMeshToScene();
        update();
    }
}

void PreviewWidget::pickEditAt(int x, int y)
{
    makeCurrent();
    GLint vp[4] = {0, 0, width(), height()};
    glGetIntegerv(GL_VIEWPORT, vp);
    GLdouble proj[16], model[16];
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    const double ygl = vp[3] - y;
    GLdouble ox, oy, oz, fx, fy, fz;
    gluUnProject(x, ygl, 0.0, model, proj, vp, &ox, &oy, &oz);
    gluUnProject(x, ygl, 1.0, model, proj, vp, &fx, &fy, &fz);
    vec<> orig(ox, oy, oz);
    vec<> dir(fx - ox, fy - oy, fz - oz);
    const double dl = dir.len();
    if (dl < 1e-12)
        return;
    dir = dir * (1.0 / dl);

    if (EditableMesh* mesh = dynamic_cast<EditableMesh*>(selectedMeshObject())) {
        int bestV = -1;
        double bestD = 1e9;
        for (int i = 0; i < mesh->vertCount(); ++i) {
            const vec<> w = mesh->worldVertex(i);
            const vec<> oc = orig - w;
            const double t = -oc.dot(dir);
            if (t < 0)
                continue;
            const vec<> closest = orig + dir * t;
            const double d = (closest - w).len();
            if (d < 0.35 && d < bestD) {
                bestD = d;
                bestV = i;
            }
        }
        m_editVert = bestV;

        int bestF = -1;
        double bestFt = 1e9;
        for (int f = 0; f < mesh->triCount(); ++f) {
            const vec<> a = mesh->worldVertex(mesh->indices[static_cast<size_t>(f * 3 + 0)]);
            const vec<> b = mesh->worldVertex(mesh->indices[static_cast<size_t>(f * 3 + 1)]);
            const vec<> c = mesh->worldVertex(mesh->indices[static_cast<size_t>(f * 3 + 2)]);
            const vec<> n = (b - a) ^ (c - a);
            const double denom = n.dot(dir);
            if (std::abs(denom) < 1e-12)
                continue;
            const double t = n.dot(a - orig) / denom;
            if (t < 0.0 || t > bestFt)
                continue;
            const vec<> p = orig + dir * t;
            const vec<> n1 = (b - a) ^ (p - a);
            const vec<> n2 = (c - b) ^ (p - b);
            const vec<> n3 = (a - c) ^ (p - c);
            if (n1.dot(n) >= -1e-8 && n2.dot(n) >= -1e-8 && n3.dot(n) >= -1e-8) {
                bestFt = t;
                bestF = f;
            }
        }
        m_editFace = bestF;
        if (m_editVert < 0 && bestF >= 0)
            m_editVert = mesh->indices[static_cast<size_t>(bestF * 3)];
        update();
        return;
    }

    FourDWireFigure* f4 = asFourDFigure(selectedMeshObject());
    if (!f4)
        return;
    std::vector<Vec4> uniq;
    f4->uniqueLocalVerts(uniq);
    const double kOff = (m_scene && m_selectedObject >= 0 && m_selectedObject < m_scene->objects.size())
                            ? m_scene->objects[m_selectedObject].pk
                            : f4->kPos;
    int bestV = -1;
    double bestScore = 1e9;
    for (int i = 0; i < static_cast<int>(uniq.size()); ++i) {
        const Vec4 w = f4->worldVert(uniq[static_cast<size_t>(i)], f4->pos, kOff);
        const vec<> xyz(w.x, w.y, w.z);
        const vec<> oc = orig - xyz;
        const double t = -oc.dot(dir);
        if (t < 0)
            continue;
        const vec<> closest = orig + dir * t;
        const double d = (closest - xyz).len();
        const double score = d + 0.35 * std::abs(w.k - m_cam4dK);
        if (d < 0.55 && score < bestScore) {
            bestScore = score;
            bestV = i;
        }
    }
    m_editVert = bestV;
    m_editFace = -1;
    update();
}

void PreviewWidget::applyOrbitFromObject(const SceneObject& o)
{
    m_targetX = o.px;
    m_targetY = o.py;
    m_targetZ = o.pz;
    m_pitch = o.rx * M_PI / 180.0;
    m_yaw = o.ry * M_PI / 180.0;
    if (m_pitch > 1.4)
        m_pitch = 1.4;
    if (m_pitch < -1.4)
        m_pitch = -1.4;
    if (!o.extra.isEmpty() && o.extra[0] > 1.0)
        m_dist = std::clamp(o.extra[0], 2.0, 500.0);
    update();
}

void PreviewWidget::writeOrbitToObject(SceneObject& o) const
{
    o.px = m_targetX;
    o.py = m_targetY;
    o.pz = m_targetZ;
    o.rx = m_pitch * 180.0 / M_PI;
    o.ry = m_yaw * 180.0 / M_PI;
    o.collide = 0;
    o.isStatic = 1;
    o.gravityMode = 0;
    if (o.extra.isEmpty())
        o.extra.append(m_dist);
    else
        o.extra[0] = m_dist;
}

void PreviewWidget::emitCameraIfNeeded()
{
    emit cameraMoved();
}

bool PreviewWidget::selectedIsCamera() const
{
    return m_scene && m_selectedObject >= 0 && m_selectedObject < m_scene->objects.size() &&
           isCameraType(m_scene->objects[m_selectedObject].type);
}

void PreviewWidget::screenRay(int x, int y, vec<>& orig, vec<>& dir) const
{
    double ex, ey, ez;
    cameraEye(ex, ey, ez);
    orig = vec<>(ex, ey, ez);
    vec<> right, up, forward;
    cameraBasis(right, up, forward);
    const double aspect = height() > 0 ? double(width()) / double(height()) : 1.0;
    const double tanHalf = std::tan(25.0 * M_PI / 180.0);
    const double nx = (2.0 * x) / std::max(1, width()) - 1.0;
    const double ny = 1.0 - (2.0 * y) / std::max(1, height());
    dir = forward + right * (nx * tanHalf * aspect) + up * (ny * tanHalf);
    const double dl = dir.len();
    if (dl > 1e-12)
        dir = dir * (1.0 / dl);
}

double PreviewWidget::gizmoSize() const
{
    return std::max(1.2, m_dist * 0.12);
}

vec<> PreviewWidget::gizmoOrigin() const
{
    if (!m_scene || m_selectedObject < 0 || m_selectedObject >= m_scene->objects.size())
        return vec<>(0, 0, 0);
    const SceneObject& o = m_scene->objects[m_selectedObject];
    return vec<>(o.px, o.py, o.pz);
}

static double raySegmentDist(const vec<>& orig, const vec<>& dir, const vec<>& a, const vec<>& b)
{
    const vec<> v = b - a;
    const vec<> w0 = orig - a;
    const double vv = v.len2();
    const double dv = dir.dot(v);
    const double denom = vv - dv * dv;
    double sc = 0, tc = 0;
    if (std::abs(denom) < 1e-12) {
        tc = 0;
        sc = std::max(0.0, -w0.dot(dir));
    } else {
        const double dw = dir.dot(w0);
        const double vw = v.dot(w0);
        sc = (dv * vw - vv * dw) / denom;
        tc = (vw - dv * dw) / denom;
        if (sc < 0)
            sc = 0;
        if (tc < 0)
            tc = 0;
        else if (tc > 1)
            tc = 1;
    }
    const vec<> p = orig + dir * sc;
    const vec<> q = a + v * tc;
    return (p - q).len();
}

static bool rayPlaneHit(const vec<>& orig, const vec<>& dir, const vec<>& center, const vec<>& n, vec<>& hit)
{
    const double dn = dir.dot(n);
    if (std::abs(dn) < 1e-9)
        return false;
    const double t = (center - orig).dot(n) / dn;
    if (t < 1e-4)
        return false;
    hit = orig + dir * t;
    return true;
}

PreviewWidget::GizmoPart PreviewWidget::hitGizmo(int x, int y) const
{
    if (m_editMode || m_playState == PlayState::Playing)
        return GizmoPart::None;
    if (!m_scene || m_selectedObject < 0 || m_selectedObject >= m_scene->objects.size())
        return GizmoPart::None;
    vec<> orig, dir;
    screenRay(x, y, orig, dir);
    const vec<> c = gizmoOrigin();
    const double len = gizmoSize();
    const double thresh = std::max(0.18, len * 0.08);
    const vec<> axes[3] = {vec<>(1, 0, 0), vec<>(0, 1, 0), vec<>(0, 0, 1)};
    const GizmoPart axisParts[3] = {GizmoPart::AxisX, GizmoPart::AxisY, GizmoPart::AxisZ};
    const GizmoPart rotParts[3] = {GizmoPart::RotX, GizmoPart::RotY, GizmoPart::RotZ};
    double best = thresh;
    GizmoPart bestPart = GizmoPart::None;
    for (int i = 0; i < 3; ++i) {
        const vec<> tip = c + axes[i] * len;
        const double d = raySegmentDist(orig, dir, c, tip);
        if (d < best) {
            best = d;
            bestPart = axisParts[i];
        }
    }
    if (bestPart != GizmoPart::None)
        return bestPart;
    const double rad = len * 0.72;
    const double ringTh = std::max(0.16, len * 0.07);
    for (int i = 0; i < 3; ++i) {
        vec<> hit;
        if (!rayPlaneHit(orig, dir, c, axes[i], hit))
            continue;
        const double d = std::abs((hit - c).len() - rad);
        if (d < ringTh && d < best) {
            best = d;
            bestPart = rotParts[i];
        }
    }
    return bestPart;
}

void PreviewWidget::applyLiveObjectTransform(int index)
{
    if (!m_scene || index < 0 || index >= m_scene->objects.size())
        return;
    if (index >= static_cast<int>(m_objects.size()) || !m_objects[static_cast<size_t>(index)])
        return;
    const SceneObject& o = m_scene->objects[index];
    based* obj = m_objects[static_cast<size_t>(index)];
    const vec<> p(o.px, o.py, o.pz);
    const vec<> s(o.sx, o.sy, o.sz);
    if (auto* e = dynamic_cast<EditorSphere*>(obj)) {
        e->pos = p;
        e->scale = s;
        e->rx = o.rx;
        e->ry = o.ry;
        e->rz = o.rz;
    } else if (auto* b = dynamic_cast<EditorBox*>(obj)) {
        b->pos = p;
        b->scale = s;
        b->rx = o.rx;
        b->ry = o.ry;
        b->rz = o.rz;
    } else if (auto* cy = dynamic_cast<EditorCylinder*>(obj)) {
        cy->pos = p;
        cy->scale = s;
        cy->rx = o.rx;
        cy->ry = o.ry;
        cy->rz = o.rz;
    } else if (auto* to = dynamic_cast<EditorTorus*>(obj)) {
        to->pos = p;
        to->scale = s;
        to->rx = o.rx;
        to->ry = o.ry;
        to->rz = o.rz;
    } else if (auto* em = dynamic_cast<EditableMesh*>(obj)) {
        em->pos = p;
        em->scale = s;
        em->rx = o.rx;
        em->ry = o.ry;
        em->rz = o.rz;
    } else if (auto* f4 = dynamic_cast<FourDWireFigure*>(obj)) {
        f4->pos = p;
        f4->scale = s;
        f4->rx = o.rx;
        f4->ry = o.ry;
        f4->rz = o.rz;
    } else if (auto* w = dynamic_cast<TransformWrapper*>(obj)) {
        w->setWorldTransform(p, s, o.rx, o.ry, o.rz);
    }
}

static void drawArrow(const vec<>& from, const vec<>& axis, double len, float r, float g, float b, bool hi)
{
    const vec<> tip = from + axis * len;
    glColor3f(hi ? std::min(1.f, r + 0.35f) : r, hi ? std::min(1.f, g + 0.35f) : g, hi ? std::min(1.f, b + 0.35f) : b);
    glLineWidth(hi ? 4.f : 2.5f);
    glBegin(GL_LINES);
    glVertex3d(from.x, from.y, from.z);
    glVertex3d(tip.x, tip.y, tip.z);
    glEnd();
    glPushMatrix();
    glTranslated(tip.x, tip.y, tip.z);
    if (std::abs(axis.x) > 0.9)
        glRotated(90, 0, 1, 0);
    else if (std::abs(axis.y) > 0.9)
        glRotated(-90, 1, 0, 0);
    glutSolidCone(len * 0.055, len * 0.18, 10, 1);
    glPopMatrix();
}

static void drawRing(const vec<>& c, const vec<>& n, double rad, float r, float g, float b, bool hi)
{
    vec<> tmp = std::abs(n.y) < 0.9 ? vec<>(0, 1, 0) : vec<>(1, 0, 0);
    vec<> u = tmp ^ n;
    const double ul = u.len();
    if (ul > 1e-9)
        u = u * (1.0 / ul);
    const vec<> v = n ^ u;
    glColor3f(hi ? std::min(1.f, r + 0.35f) : r, hi ? std::min(1.f, g + 0.35f) : g, hi ? std::min(1.f, b + 0.35f) : b);
    glLineWidth(hi ? 3.5f : 2.0f);
    glBegin(GL_LINE_LOOP);
    const int segs = 64;
    for (int i = 0; i < segs; ++i) {
        const double a = (2.0 * M_PI * i) / segs;
        const vec<> p = c + u * (rad * std::cos(a)) + v * (rad * std::sin(a));
        glVertex3d(p.x, p.y, p.z);
    }
    glEnd();
}

void PreviewWidget::drawGizmos()
{
    if (!m_scene || m_selectedObject < 0 || m_selectedObject >= m_scene->objects.size())
        return;
    const vec<> c = gizmoOrigin();
    const double len = gizmoSize();
    const GizmoPart hot = (m_gizmoDrag != GizmoPart::None) ? m_gizmoDrag : m_gizmoHover;
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    drawArrow(c, vec<>(1, 0, 0), len, 0.92f, 0.22f, 0.22f, hot == GizmoPart::AxisX);
    drawArrow(c, vec<>(0, 1, 0), len, 0.25f, 0.85f, 0.28f, hot == GizmoPart::AxisY);
    drawArrow(c, vec<>(0, 0, 1), len, 0.25f, 0.45f, 0.95f, hot == GizmoPart::AxisZ);
    const double rad = len * 0.72;
    drawRing(c, vec<>(1, 0, 0), rad, 0.92f, 0.22f, 0.22f, hot == GizmoPart::RotX);
    drawRing(c, vec<>(0, 1, 0), rad, 0.25f, 0.85f, 0.28f, hot == GizmoPart::RotY);
    drawRing(c, vec<>(0, 0, 1), rad, 0.25f, 0.45f, 0.95f, hot == GizmoPart::RotZ);
    glPopAttrib();
}

