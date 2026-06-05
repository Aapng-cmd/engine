#pragma once

#include "SceneFile.h"

#include <QOpenGLWidget>
#include <QPoint>
#include <vector>

class QKeyEvent;

struct based;

/** OpenGL preview with orbit camera and object picking (bounding spheres). */
class PreviewWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);
    ~PreviewWidget() override;

    void setRepoRoot(const QString& root);
    void setSceneData(SceneData* data);
    void markSceneDirty();

signals:
    void objectPicked(int index);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private:
    void rebuildObjectsAndTextures();
    void pickAt(int x, int y);
    void cameraEye(double& ex, double& ey, double& ez) const;

    QString m_repoRoot;
    SceneData* m_scene = nullptr;
    bool m_dirty = true;

    std::vector<GLuint> m_texIds;
    std::vector<based*> m_objects;

    double m_targetX = 0, m_targetY = 2, m_targetZ = 0;
    double m_dist = 35;
    double m_yaw = 0.5;
    double m_pitch = 0.35;

    QPoint m_lastPos;
    QPoint m_pressPos;
    bool m_rotating = false;
    bool m_panning = false;
    bool m_glutInited = false;
};
