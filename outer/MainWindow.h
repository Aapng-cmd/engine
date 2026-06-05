#pragma once

#include "SceneFile.h"

#include <QMainWindow>

class QListWidget;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
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
    QComboBox* m_texCombo = nullptr;
    QLabel* m_typeLabel = nullptr;

    static constexpr int kMaxExtras = 12;
    QLabel* m_extraLabel[kMaxExtras] = {};
    QDoubleSpinBox* m_extraSpin[kMaxExtras] = {};

    bool m_blockSignals = false;
};
