#include "PreviewWidget.h"

#include "object_factory.h"
#include "figures.h"
#include "textures.h"

#include <GL/glut.h>
#include <GL/glu.h>

#include <QDir>
#include <QFileInfo>
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

void PreviewWidget::cameraEye(double& ex, double& ey, double& ez) const
{
    double cp = cos(m_pitch), sp = sin(m_pitch);
    double cy = cos(m_yaw), sy = sin(m_yaw);
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
    m_texIds.clear();

    if (!m_scene)
        return;

    for (const QString& tp : m_scene->textures) {
        QString path = resolveTexturePath(m_repoRoot, tp);
        GLuint id = LoadTexID(path.toStdString());
        m_texIds.push_back(id);
    }

    for (const SceneObject& o : m_scene->objects) {
        std::vector<double> ex;
        for (double v : o.extra)
            ex.push_back(v);
        GLuint tex = 0;
        if (o.texIndex >= 0 && o.texIndex < static_cast<int>(m_texIds.size()))
            tex = m_texIds[static_cast<size_t>(o.texIndex)];

        std::string err;
        based* obj = createSceneObject(o.type.toStdString(), o.px, o.py, o.pz, o.sx, o.sy, o.sz, o.rx, o.ry, o.rz, ex,
                                       tex, &err);
        if (obj)
            m_objects.push_back(obj);
    }
}

static bool raySphere(vec<> orig, vec<> dir, vec<> center, double R, double& tHit)
{
    vec<> oc = orig - center;
    double b = oc.x * dir.x + oc.y * dir.y + oc.z * dir.z;
    double c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - R * R;
    double disc = b * b - c;
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
    double dl = dir.len();
    if (dl < 1e-9)
        return;
    dir = dir * (1.0 / dl);

    int best = -1;
    double bestT = std::numeric_limits<double>::infinity();
    for (int i = 0; i < static_cast<int>(m_objects.size()); ++i) {
        vec<> c;
        double r = 0;
        m_objects[static_cast<size_t>(i)]->getBoundingSphere(c, r, 0);
        double t;
        if (raySphere(orig, dir, c, r, t) && t < bestT) {
            bestT = t;
            best = i;
        }
    }
    if (best >= 0)
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

    glEnable(GL_DEPTH_TEST);
    for (based* p : m_objects)
        p->Draw(0);

    glDisable(GL_LIGHTING);
    glColor3f(0.35f, 0.45f, 0.55f);
    glBegin(GL_LINES);
    const double G = 80;
    for (double x = -G; x <= G; x += 5) {
        glVertex3d(x, 0, -G);
        glVertex3d(x, 0, G);
    }
    for (double z = -G; z <= G; z += 5) {
        glVertex3d(-G, 0, z);
        glVertex3d(G, 0, z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void PreviewWidget::keyPressEvent(QKeyEvent* e)
{
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
        double fl = F.len();
        if (fl < 1e-9)
            return;
        F = F * (1.0 / fl);
        vec<> Wup(0, 1, 0);
        vec<> Rv = Wup ^ F;
        double rl = Rv.len();
        if (rl < 1e-9)
            return;
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
