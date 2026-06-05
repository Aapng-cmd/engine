#pragma once

#include "SceneFile.h"

#include <QMainWindow>
#include <QVector>

class QListWidget;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QProcess;
class QListWidgetItem;
class QToolBox;
class PreviewWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpen();
    void onSave();
    void onSaveAs();
    void onBuildViewer();
    void onAddTexture();
    void onRemoveTexture();
    void addFigure(const QString& type);
    void onRemoveObject();
    void onObjectSelectionChanged();
    void onPreviewObjectPicked(int index);
    void applyTransformFromUi();
    void onTextureIndexChanged(int index);
    void onRescanTextures();
    void onMergeSelected();
    void onObjectItemActivated(QListWidgetItem* item);

private:
    void buildUi();
    void loadFile(const QString& path);
    bool saveToFile(const QString& path);
    void refreshObjectList();
    void refreshTextureList();
    void refreshTextureCombo();
    void loadObjectIntoUi(int row);
    void pushUiToObject(int row);
    void setExtraEditorsForType(const QString& type);
    void syncPreviewRoot();
    void markPreviewDirty();
    /** Remap texture indices after syncing the list with files in repo textures/. */
    void refreshTexturesFromFolder();
    QString repoRoot() const;
    QString toProjectRelative(const QString& absolutePath) const;

    SceneData m_data;
    QString m_currentFile;

    QListWidget* m_objectList = nullptr;
    QListWidget* m_textureList = nullptr;
    PreviewWidget* m_preview = nullptr;

    QDoubleSpinBox* m_px = nullptr;
    QDoubleSpinBox* m_py = nullptr;
    QDoubleSpinBox* m_pz = nullptr;
    QDoubleSpinBox* m_sx = nullptr;
    QDoubleSpinBox* m_sy = nullptr;
    QDoubleSpinBox* m_sz = nullptr;
    QDoubleSpinBox* m_rx = nullptr;
    QDoubleSpinBox* m_ry = nullptr;
    QDoubleSpinBox* m_rz = nullptr;
    QDoubleSpinBox* m_vx = nullptr;
    QDoubleSpinBox* m_vy = nullptr;
    QDoubleSpinBox* m_vz = nullptr;
    QDoubleSpinBox* m_orbitX = nullptr;
    QDoubleSpinBox* m_orbitY = nullptr;
    QDoubleSpinBox* m_orbitZ = nullptr;
    QDoubleSpinBox* m_orbitOmega = nullptr;
    QDoubleSpinBox* m_groupId = nullptr;
    QComboBox* m_useGravity = nullptr;
    QComboBox* m_useFriction = nullptr;
    QDoubleSpinBox* m_gx = nullptr;
    QDoubleSpinBox* m_gy = nullptr;
    QDoubleSpinBox* m_gz = nullptr;
    QDoubleSpinBox* m_groundFriction = nullptr;
    QDoubleSpinBox* m_restitution = nullptr;
    QComboBox* m_texCombo = nullptr;
    QLabel* m_typeLabel = nullptr;

    static constexpr int kMaxExtras = 12;
    QLabel* m_extraLabel[kMaxExtras] = {};
    QDoubleSpinBox* m_extraSpin[kMaxExtras] = {};

    bool m_blockSignals = false;
    QProcess* m_buildProc = nullptr;
    QString m_buildLog;
    QVector<int> m_rowToObject;
};
