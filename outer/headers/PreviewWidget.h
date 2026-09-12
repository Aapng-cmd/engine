#pragma once

#include "SceneFile.h"
#include "vector.h"

#include <QOpenGLWidget>
#include <QPoint>
#include <vector>

class QKeyEvent;
class QTimer;
struct based;
struct Scene;

/** OpenGL preview with orbit camera, Play/Pause/Stop, and mesh edit. */
class PreviewWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    enum class PlayState { Stopped, Playing, Paused };

    explicit PreviewWidget(QWidget* parent = nullptr);
    ~PreviewWidget() override;

    void setRepoRoot(const QString& root);
    void setSceneData(SceneData* data);
    void markSceneDirty();
    void setSelectedObject(int index);

    void startPlay();
    void pausePlay();
    void stopPlay();
    PlayState playState() const { return m_playState; }

    void setSliceMode(bool slice);
    bool sliceMode() const { return m_sliceMode; }
    void setCam4dK(double k);
    double cam4dK() const { return m_cam4dK; }
    void applyOrbitFromObject(const SceneObject& o);
    void writeOrbitToObject(SceneObject& o) const;

    void setEditMode(bool on);
    bool editMode() const { return m_editMode; }
    void setDebugLayer(int layer);
    int debugLayer() const { return m_debugLayer; }
    void meshAddCube();
    void meshAddPlane();
    void meshExtrudeSelected();
    void syncSelectedMeshToScene();
    void syncSelectedTetsToScene();
    bool convertSelectedToMesh();

signals:
    void objectPicked(int index);
    void meshEdited(int index);
    void playStateChanged();
    void debugLayerChanged(int layer);
    void transformEdited(int index);
    void cameraMoved();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private slots:
    void onPlayTick();

private:
    void rebuildObjectsAndTextures();
    void applyObjectExtras(based* obj, const SceneObject& o);
    void pickAt(int x, int y);
    void cameraEye(double& ex, double& ey, double& ez) const;
    void cameraBasis(vec<>& right, vec<>& up, vec<>& forward) const;
    void clearPlayScene();
    void buildPlayScene();
    void pickEditAt(int x, int y);
    based* selectedMeshObject() const;
    void drawCollisionWire(based* obj, int subdiv, float r, float g, float b);
    void drawEditTimeDynamics();
    void drawDebugOverlays();
    void screenRay(int x, int y, vec<>& orig, vec<>& dir) const;
    double gizmoSize() const;
    vec<> gizmoOrigin() const;
    enum class GizmoPart { None, AxisX, AxisY, AxisZ, RotX, RotY, RotZ };
    GizmoPart hitGizmo(int x, int y) const;
    void drawGizmos();
    void applyLiveObjectTransform(int index);
    bool selectedIsCamera() const;
    void emitCameraIfNeeded();

    QString m_repoRoot;
    SceneData* m_scene = nullptr;
    bool m_dirty = true;

    std::vector<GLuint> m_texIds;
    std::vector<based*> m_objects;
    based* m_ground = nullptr;
    based* m_sky = nullptr;

    double m_targetX = 0, m_targetY = 2, m_targetZ = 0;
    double m_dist = 35;
    double m_yaw = 0.5;
    double m_pitch = 0.35;

    QPoint m_lastPos;
    QPoint m_pressPos;
    bool m_rotating = false;
    bool m_panning = false;
    bool m_glutInited = false;
    int m_selectedObject = -1;
    bool m_use4dCamera = false;
    bool m_userCameraOverride = false;
    double m_cam4dK = 0.0;
    bool m_sliceMode = true;

    PlayState m_playState = PlayState::Stopped;
    QTimer* m_playTimer = nullptr;
    Scene* m_playScene = nullptr;

    bool m_editMode = false;
    int m_editVert = -1;
    int m_editFace = -1;
    bool m_grabbing = false;
    int m_debugLayer = 0;

    GizmoPart m_gizmoHover = GizmoPart::None;
    GizmoPart m_gizmoDrag = GizmoPart::None;
    vec<> m_dragStartPos;
    double m_dragStartRx = 0, m_dragStartRy = 0, m_dragStartRz = 0;
    double m_dragAxisT0 = 0;
    double m_dragStartAngle = 0;
    double m_dragStartYaw = 0, m_dragStartPitch = 0;
    double m_dragStartDist = 35;
};
