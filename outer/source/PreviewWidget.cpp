#include "PreviewWidget.h"

#include "collision_mesh.h"
#include "collision_repr.h"
#include "object_factory.h"
#include "figures.h"
#include "fourd_figure.h"
#include "fourd_math.h"
#include "render_material.h"
#include "manual_shapes.h"
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
#include <QWheelEvent>
#include <cmath>
#include <limits>

PreviewWidget::PreviewWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

PreviewWidget::~PreviewWidget()
{
    makeCurrent();
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
    markSceneDirty();
}

void PreviewWidget::setSceneData(SceneData* data)
{
    m_scene = data;
    markSceneDirty();
}

void PreviewWidget::markSceneDirty()
{
    m_dirty = true;
    update();
}

void PreviewWidget::setSelectedObject(int index)
{
    m_selectedObject = index;
    if (!m_userCameraOverride && m_scene && index >= 0 && index < m_scene->objects.size()) {
        const QString& tp = m_scene->objects[index].type;
        m_use4dCamera = (tp == QLatin1String("tesseract") || tp == QLatin1String("hypersphere") ||
                         tp == QLatin1String("pyramid4d"));
    } else if (!m_userCameraOverride)
        m_use4dCamera = false;
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
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat amb[] = {0.35f, 0.35f, 0.4f, 1.f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
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
    auto* gp = new GroundPlane(envTex(m_scene->env.groundTexture), m_scene->env.groundEdge1, m_scene->env.groundEdge2);
    const bool water = m_scene->env.groundTexture.contains(QStringLiteral("water"), Qt::CaseInsensitive);
    gp->setReflect(water ? 1.0 : 0.0, water);
    m_ground = gp;
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
    if (m_dirty) {
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
    Camera4DState cam4;
    const vec<> eye(ex, ey, ez);
    const vec<> fwd(m_targetX - ex, m_targetY - ey, m_targetZ - ez);
    cam4.location.k = m_cam4dK;
    fourd::syncViewerToCamera4d(cam4, eye, fwd);
    double selK = 0.0;
    if (m_scene && m_selectedObject >= 0 && m_selectedObject < m_scene->objects.size())
        selK = m_scene->objects[m_selectedObject].pk;
    vec<> kBase, kTip;
    if (fourd::projectTo3D(cam4, {0, 0, 0, selK}, kBase) &&
        fourd::projectTo3D(cam4, {0, 0, 0, selK + 3}, kTip)) {
        glColor3f(1, 0, 1);
        glBegin(GL_LINES);
        glVertex3d(kBase.x, kBase.y, kBase.z);
        glVertex3d(kTip.x, kTip.y, kTip.z);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    if (m_sky)
        m_sky->Draw(0);
    glEnable(GL_LIGHTING);
    for (size_t i = 0; i < m_objects.size(); ++i) {
        double a = 1.0;
        if (m_scene && static_cast<int>(i) < m_scene->objects.size())
            a = m_scene->objects[static_cast<int>(i)].alpha;
        setFigureRenderAlpha(m_objects[i], a);
        const AlphaReflect ar = decomposeAlphaReflect(a);
        applyFigureMaterial(ar.opacity, ar.reflect);
        const bool transparent = ar.opacity < 0.999;
        if (transparent)
            glDepthMask(GL_FALSE);
        double kW = 0.0;
        if (m_scene && static_cast<int>(i) < m_scene->objects.size())
            kW = m_scene->objects[static_cast<int>(i)].pk;
        if (m_use4dCamera) {
            if (auto* f4 = dynamic_cast<FourDWireFigure*>(m_objects[i]))
                f4->drawProjected(cam4, kW);
            else
                m_objects[i]->Draw(0);
        } else if (auto* f4 = dynamic_cast<FourDWireFigure*>(m_objects[i]))
            f4->drawProjected(cam4, kW);
        else
            m_objects[i]->Draw(0);
        if (transparent)
            glDepthMask(GL_TRUE);
    }

    if (m_selectedObject >= 0 && m_selectedObject < static_cast<int>(m_objects.size())) {
        based* sel = m_objects[static_cast<size_t>(m_selectedObject)];
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glLineWidth(2.5f);
        glColor3f(0.2f, 0.55f, 1.0f);

        if (usesTriangleCollision(collisionReprForObject(sel))) {
            double ex, ey, ez;
            cameraEye(ex, ey, ez);
            const vec<> cam(ex, ey, ez);
            vec<> anchor(0, 0, 0);
            if (m_scene && m_selectedObject < m_scene->objects.size()) {
                const SceneObject& so = m_scene->objects[m_selectedObject];
                anchor = vec<>(so.px, so.py, so.pz);
            }
            double faceSize = 1.0;
            if (auto* bx = dynamic_cast<EditorBox*>(sel))
                faceSize = 2.0 * std::min({0.5 * std::abs(bx->dx * bx->scale.x), 0.5 * std::abs(bx->dy * bx->scale.y),
                                             0.5 * std::abs(bx->dz * bx->scale.z)});
            else if (auto* w = dynamic_cast<TransformWrapper*>(sel)) {
                if (auto* sc = dynamic_cast<SolidCube*>(w->getChild()))
                    faceSize = 2.0 * std::min({sc->hx, sc->hy, sc->hz});
            }
            const int subdiv = collision::lodFaceSubdiv(faceSize, (anchor - cam).len());
            std::vector<CollTri> tris;
            collision::buildObjectCollisionMesh(sel, tris, subdiv);
            for (const CollTri& tri : tris) {
                glBegin(GL_LINE_LOOP);
                glVertex3d(tri.v0.x, tri.v0.y, tri.v0.z);
                glVertex3d(tri.v1.x, tri.v1.y, tri.v1.z);
                glVertex3d(tri.v2.x, tri.v2.y, tri.v2.z);
                glEnd();
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

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    if (m_ground)
        m_ground->Draw(0);
    glEnable(GL_LIGHTING);
}

void PreviewWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_T) {
        m_use4dCamera = !m_use4dCamera;
        m_userCameraOverride = true;
        update();
        e->accept();
        return;
    }
    if (m_use4dCamera) {
        if (e->key() == Qt::Key_Q) {
            m_cam4dK -= 0.35;
            update();
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_E) {
            m_cam4dK += 0.35;
            update();
            e->accept();
            return;
        }
    }
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Plus || e->key() == Qt::Key_Equal) {
            m_dist *= 0.9;
            if (m_dist < 2)
                m_dist = 2;
            update();
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_Minus || e->key() == Qt::Key_Underscore) {
            m_dist /= 0.9;
            if (m_dist > 500)
                m_dist = 500;
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
    if (e->button() == Qt::LeftButton)
        m_rotating = true;
    else if (e->button() == Qt::RightButton)
        m_panning = true;
}

void PreviewWidget::mouseMoveEvent(QMouseEvent* e)
{
    QPoint d = e->pos() - m_lastPos;
    m_lastPos = e->pos();
    if (m_rotating) {
        m_yaw -= d.x() * 0.01;
        m_pitch += d.y() * 0.01;
        const double lim = 1.4;
        if (m_pitch > lim)
            m_pitch = lim;
        if (m_pitch < -lim)
            m_pitch = -lim;
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
        update();
    }
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        if ((e->pos() - m_pressPos).manhattanLength() < 5) {
            pickAt(e->pos().x(), e->pos().y());
        }
        m_rotating = false;
    }
    if (e->button() == Qt::RightButton)
        m_panning = false;
}

void PreviewWidget::wheelEvent(QWheelEvent* e)
{
    double steps = e->angleDelta().y() / 120.0;
    m_dist *= std::pow(0.92, steps);
    if (m_dist < 2)
        m_dist = 2;
    if (m_dist > 500)
        m_dist = 500;
    update();
}
