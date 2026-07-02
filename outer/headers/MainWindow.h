#pragma once

#include "SceneFile.h"
#include "CustomFigures.h"

#include <QMainWindow>
#include <QVector>

class QVBoxLayout;
class QListWidget;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QProcess;
class QListWidgetItem;
class QToolBox;
class PreviewWidget;
class QSlider;
class QGroupBox;

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
    void addFigureFromPreset(const CustomFigurePreset& preset);
    void onRemoveObject();
    void onObjectSelectionChanged();
    void onPreviewObjectPicked(int index);
    void applyTransformFromUi();
    void onTextureIndexChanged(int index);
    void onRescanTextures();
    void onMergeSelected();
    void onObjectItemActivated(QListWidgetItem* item);
    void onSaveCustomFigure();

private:
    void buildUi();
    void loadFile(const QString& path);
    bool saveToFile(const QString& path);
    void refreshObjectList();
    void updateObjectListRow(int visRow, int objRow);
    void refreshCustomFigureButtons();
    QString objectListLabel(int objRow) const;
    int visibleRowForObject(int objRow) const;
    void selectObjectRow(int objRow);
    void refreshTextureList();
    void refreshTextureCombo();
    void loadObjectIntoUi(int row);
    void pushUiToObject(int row);
    void setExtraEditorsForType(const QString& type);
    void syncGravityPanels(int mode);
    void refreshCollisionPolyCount();
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
    QDoubleSpinBox* m_pk = nullptr;
    QDoubleSpinBox* m_vk = nullptr;
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
    QComboBox* m_collide = nullptr;
    QDoubleSpinBox* m_alpha = nullptr;
    QDoubleSpinBox* m_mass = nullptr;
    QDoubleSpinBox* m_gravTargetX = nullptr;
    QDoubleSpinBox* m_gravTargetY = nullptr;
    QDoubleSpinBox* m_gravTargetZ = nullptr;
    QDoubleSpinBox* m_gravStrength = nullptr;
    QDoubleSpinBox* m_gravTargetObject = nullptr;
    QSlider* m_collisionSubdiv = nullptr;
    QLabel* m_collisionPolyCount = nullptr;
    QGroupBox* m_primitiveGravBox = nullptr;
    QGroupBox* m_advancedGravBox = nullptr;
    QComboBox* m_texCombo = nullptr;
    QLabel* m_typeLabel = nullptr;

    static constexpr int kMaxExtras = 12;
    QLabel* m_extraLabel[kMaxExtras] = {};
    QDoubleSpinBox* m_extraSpin[kMaxExtras] = {};

    bool m_blockSignals = false;
    /** Object index last loaded into the property panel (for save-on-switch). */
    int m_editingObjectRow = -1;
    QProcess* m_buildProc = nullptr;
    QString m_buildLog;
    QVector<int> m_rowToObject;
    QVector<CustomFigurePreset> m_customFigures;
    class QVBoxLayout* m_customFiguresLayout = nullptr;
    QString m_customCatalogPath;
};
