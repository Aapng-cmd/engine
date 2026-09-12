#include "MainWindow.h"
#include "object_factory.h"
#include "collision_mesh.h"
#include "editable_mesh.h"
#include "fourd_figure.h"
#include "engine_power.h"
#include "CustomFigures.h"
#include "PreviewWidget.h"
#include "ProjectRoot.h"
#include "EditorPrefs.h"

#include <QActionGroup>
#include <QApplication>
#include <QColorDialog>
#include <QCoreApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QTextCursor>
#include <QPushButton>
#include <QStringList>
#include <QInputDialog>
#include <QSet>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSlider>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolBox>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <utility>

static QStringList scanRepoTexturesFolder(const QString& repoRoot)
{
    QStringList out;
    QDir dir(QDir(repoRoot).filePath(QStringLiteral("textures")));
    if (!dir.exists())
        return out;
    static const QStringList kExt = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("bmp"),
        QStringLiteral("tga"), QStringLiteral("webp"),
    };
    const QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo& fi : files) {
        if (kExt.contains(fi.suffix().toLower()))
            out.append(QStringLiteral("textures/") + fi.fileName());
    }
    return out;
}

static void setupSpin(QDoubleSpinBox* s, double lo = -10000, double hi = 10000)
{
    s->setRange(lo, hi);
    s->setDecimals(4);
    s->setSingleStep(0.1);
}

static int extraCountForType(const QString& type)
{
    const int n = expectedExtraCount(type.toStdString());
    return n >= 0 ? n : 0;
}

static int subdivHi()
{
    return engine::maxCollisionSubdiv();
}

static QString powerItemLabel(int i)
{
    switch (i) {
    case 1:
        return i18n("1 — very low");
    case 2:
        return i18n("2 — current PC");
    case 5:
        return i18n("5 — mid");
    case 8:
        return i18n("8 — high");
    case 10:
        return i18n("10 — 32GB / RTX 4090");
    default:
        return QString::number(i);
    }
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(i18n("Scene Editor"));
    resize(1440, 900);
    setMinimumSize(960, 600);
    buildUi();

    m_preview->setSceneData(&m_data);
    syncPreviewRoot();
    m_customCatalogPath = defaultCustomFiguresCatalogPath(repoRoot());
    loadCustomFiguresCatalog(m_customCatalogPath, m_customFigures);
    refreshCustomFigureButtons();

    const QString def = QDir(resolveDriverTestRoot()).filePath(QStringLiteral("inner/default.scene"));
    if (QFileInfo::exists(def))
        loadFile(QFileInfo(def).canonicalFilePath());
    else {
        ensureCameraObject();
        refreshObjectList();
        if (!m_data.objects.isEmpty())
            selectObjectRow(0);
        markPreviewDirty();
        refreshTexturesFromFolder();
    }
}

void MainWindow::syncPreviewRoot()
{
    m_preview->setRepoRoot(repoRoot());
}

void MainWindow::markPreviewDirty()
{
    m_preview->markSceneDirty();
}

void MainWindow::buildUi()
{
    auto* openAct = new QAction(this);
    markI18n(openAct, "Open…");
    auto* saveAct = new QAction(this);
    markI18n(saveAct, "Save");
    auto* saveAsAct = new QAction(this);
    markI18n(saveAsAct, "Save as…");
    auto* buildAct = new QAction(this);
    markI18n(buildAct, "Build viewer (clean + make)…");
    auto* quitAct = new QAction(this);
    markI18n(quitAct, "Quit");
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpen);
    connect(saveAct, &QAction::triggered, this, &MainWindow::onSave);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(buildAct, &QAction::triggered, this, &MainWindow::onBuildViewer);
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    QMenu* file = menuBar()->addMenu(i18n("File"));
    markI18n(file, "File");
    file->addAction(openAct);
    file->addAction(saveAct);
    file->addAction(saveAsAct);
    file->addSeparator();
    file->addAction(buildAct);
    file->addSeparator();
    file->addAction(quitAct);

    QMenu* gameObject = menuBar()->addMenu(i18n("GameObject"));
    markI18n(gameObject, "GameObject");
    const struct {
        const char* name;
        const char* type;
    } goItems[] = {
        { "Sphere", "sphere" },
        { "Cube", "cube" },
        { "Cylinder", "cylinder" },
        { "Torus", "torus" },
        { "Mesh", "mesh" },
        { "Unit cube", "solid_cube" },
        { "Cone", "cone" },
        { "Pyramid", "pyramid" },
    };
    for (const auto& f : goItems) {
        const QString t = QString::fromLatin1(f.type);
        auto* a = new QAction(this);
        markI18n(a, f.name);
        connect(a, &QAction::triggered, this, [this, t]() { addFigure(t); });
        gameObject->addAction(a);
    }

    QMenu* viewMenu = menuBar()->addMenu(i18n("View"));
    markI18n(viewMenu, "View");
    auto* maxAct = new QAction(this);
    markI18n(maxAct, "Maximize");
    connect(maxAct, &QAction::triggered, this, [this]() { showMaximized(); });
    viewMenu->addAction(maxAct);
    auto* fsAct = new QAction(this);
    markI18n(fsAct, "Full Screen");
    fsAct->setShortcut(QKeySequence(Qt::Key_F11));
    connect(fsAct, &QAction::triggered, this, &MainWindow::toggleFullScreen);
    viewMenu->addAction(fsAct);
    viewMenu->addSeparator();

    m_overlayGroup = new QActionGroup(this);
    m_overlayGroup->setExclusive(true);
    m_overlayOffAct = new QAction(this);
    markI18n(m_overlayOffAct, "Overlay off");
    m_overlayOffAct->setCheckable(true);
    m_overlayOffAct->setChecked(true);
    m_overlayBoundsAct = new QAction(this);
    markI18n(m_overlayBoundsAct, "Collision bounds");
    m_overlayBoundsAct->setCheckable(true);
    m_overlayComAct = new QAction(this);
    markI18n(m_overlayComAct, "Centers of mass & velocity");
    m_overlayComAct->setCheckable(true);
    m_overlayGroup->addAction(m_overlayOffAct);
    m_overlayGroup->addAction(m_overlayBoundsAct);
    m_overlayGroup->addAction(m_overlayComAct);
    viewMenu->addAction(m_overlayOffAct);
    viewMenu->addAction(m_overlayBoundsAct);
    viewMenu->addAction(m_overlayComAct);
    markI18nTip(viewMenu, "Same overlays as ';' in scene_viewer (also during Play).");
    connect(m_overlayOffAct, &QAction::triggered, this, [this]() { onOverlayLayerChosen(0); });
    connect(m_overlayBoundsAct, &QAction::triggered, this, [this]() { onOverlayLayerChosen(1); });
    connect(m_overlayComAct, &QAction::triggered, this, [this]() { onOverlayLayerChosen(2); });

    auto* settingsAct = new QAction(this);
    markI18n(settingsAct, "Settings");
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettings);
    menuBar()->addAction(settingsAct);

    auto* playBar = addToolBar(i18n("Play"));
    markI18n(playBar, "Play");
    playBar->setMovable(false);
    auto* playAct = new QAction(this);
    markI18n(playAct, "Play");
    auto* pauseAct = new QAction(this);
    markI18n(pauseAct, "Pause");
    auto* stopAct = new QAction(this);
    markI18n(stopAct, "Stop");
    playBar->addAction(playAct);
    playBar->addAction(pauseAct);
    playBar->addAction(stopAct);
    connect(playAct, &QAction::triggered, this, &MainWindow::onPlay);
    connect(pauseAct, &QAction::triggered, this, &MainWindow::onPause);
    connect(stopAct, &QAction::triggered, this, &MainWindow::onStop);
    playBar->addSeparator();
    playBar->addAction(buildAct);
    playBar->addSeparator();
    playBar->addWidget(makeI18nLabel(" Power "));
    m_powerCombo = new QComboBox;
    for (int i = 1; i <= 10; ++i) {
        m_powerCombo->addItem(powerItemLabel(i), i);
        switch (i) {
        case 1:
            m_powerCombo->setItemData(i - 1, QStringLiteral("1 — very low"), Qt::UserRole + 1);
            break;
        case 2:
            m_powerCombo->setItemData(i - 1, QStringLiteral("2 — current PC"), Qt::UserRole + 1);
            break;
        case 5:
            m_powerCombo->setItemData(i - 1, QStringLiteral("5 — mid"), Qt::UserRole + 1);
            break;
        case 8:
            m_powerCombo->setItemData(i - 1, QStringLiteral("8 — high"), Qt::UserRole + 1);
            break;
        case 10:
            m_powerCombo->setItemData(i - 1, QStringLiteral("10 — 32GB / RTX 4090"), Qt::UserRole + 1);
            break;
        default:
            break;
        }
    }
    m_powerCombo->setCurrentIndex(std::clamp(engine::powerLevel(), 1, 10) - 1);
    markI18nTip(m_powerCombo, "1 = cheapest, 2 = current PC, 10 = 32GB / RTX 4090-class");
    playBar->addWidget(m_powerCombo);
    connect(m_powerCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        applyEnginePower(m_powerCombo->currentData().toInt());
    });
    playBar->addSeparator();
    playBar->addWidget(makeI18nLabel("Overlay"));
    m_overlayCombo = new QComboBox;
    addI18nComboItem(m_overlayCombo, "Overlay off", 0);
    addI18nComboItem(m_overlayCombo, "Collision bounds", 1);
    addI18nComboItem(m_overlayCombo, "Centers of mass & velocity", 2);
    markI18nTip(m_overlayCombo, "Same overlays as ';' in scene_viewer (also during Play).");
    playBar->addWidget(m_overlayCombo);
    connect(m_overlayCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_blockSignals)
            return;
        onOverlayLayerChosen(m_overlayCombo->itemData(idx).toInt());
    });

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outerLay = new QVBoxLayout(central);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->setSpacing(0);

    auto* mainSplit = new QSplitter(Qt::Horizontal, this);
    mainSplit->setChildrenCollapsible(false);
    mainSplit->setHandleWidth(4);

    auto* mid = new QWidget;
    mid->setMinimumWidth(180);
    mid->setMaximumWidth(380);
    auto* midLay = new QVBoxLayout(mid);
    midLay->setContentsMargins(6, 6, 6, 6);
    midLay->addWidget(makeI18nLabel("Hierarchy"));
    m_objectList = new QListWidget;
    m_objectList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    midLay->addWidget(m_objectList);

    auto* primBox = new QGroupBox;
    markI18n(primBox, "Primitives");
    auto* primGrid = new QGridLayout(primBox);
    auto* bSph = new QPushButton;
    markI18n(bSph, "Sphere");
    auto* bBox = new QPushButton;
    markI18n(bBox, "Cube");
    auto* bCyl = new QPushButton;
    markI18n(bCyl, "Cylinder");
    auto* bTor = new QPushButton;
    markI18n(bTor, "Torus");
    auto* bMesh = new QPushButton;
    markI18n(bMesh, "Mesh");
    for (auto* b : {bSph, bBox, bCyl, bTor, bMesh}) {
        b->setAutoDefault(false);
        b->setDefault(false);
    }
    primGrid->addWidget(bSph, 0, 0);
    primGrid->addWidget(bBox, 0, 1);
    primGrid->addWidget(bCyl, 1, 0);
    primGrid->addWidget(bTor, 1, 1);
    primGrid->addWidget(bMesh, 2, 0);
    midLay->addWidget(primBox);

    auto* figBox = new QGroupBox;
    markI18n(figBox, "Figures (basic shapes)");
    auto* figGrid = new QGridLayout(figBox);
    const struct {
        const char* label;
        const char* type;
    } figRows[] = {
        { "Unit cube", "solid_cube" },
        { "Cone", "cone" },
        { "Pyramid", "pyramid" },
    };
    for (int i = 0; i < 3; ++i) {
        auto* fb = new QPushButton;
        markI18n(fb, figRows[i].label);
        fb->setAutoDefault(false);
        fb->setDefault(false);
        const QString tt = QString::fromLatin1(figRows[i].type);
        connect(fb, &QPushButton::clicked, this, [this, tt]() { addFigure(tt); });
        figGrid->addWidget(fb, i / 3, i % 3);
    }
    midLay->addWidget(figBox);

    auto* fourdBox = new QGroupBox;
    markI18n(fourdBox, "4D figures");
    auto* fourdGrid = new QGridLayout(fourdBox);
    const struct {
        const char* label;
        const char* type;
    } fourdRows[] = {
        { "Tesseract", "tesseract" },
        { "Hypersphere", "hypersphere" },
        { "5-cell", "pyramid4d" },
        { "16-cell", "16cell" },
    };
    for (int i = 0; i < 4; ++i) {
        auto* fb = new QPushButton;
        markI18n(fb, fourdRows[i].label);
        fb->setAutoDefault(false);
        fb->setDefault(false);
        const QString tt = QString::fromLatin1(fourdRows[i].type);
        connect(fb, &QPushButton::clicked, this, [this, tt]() { addFigure(tt); });
        fourdGrid->addWidget(fb, 0, i);
    }
    fourdBox->hide();
    midLay->addWidget(fourdBox);

    auto* customBox = new QGroupBox;
    markI18n(customBox, "Custom figures (saved)");
    m_customFiguresLayout = new QVBoxLayout(customBox);
    midLay->addWidget(customBox);
    auto* saveCustomBtn = new QPushButton;
    markI18n(saveCustomBtn, "Save selected as custom…");
    saveCustomBtn->setAutoDefault(false);
    saveCustomBtn->setDefault(false);
    connect(saveCustomBtn, &QPushButton::clicked, this, &MainWindow::onSaveCustomFigure);
    midLay->addWidget(saveCustomBtn);

    auto* rmObj = new QPushButton;
    markI18n(rmObj, "Remove selected");
    auto* mergeObj = new QPushButton;
    markI18n(mergeObj, "Merge selected");
    rmObj->setAutoDefault(false);
    mergeObj->setAutoDefault(false);
    midLay->addWidget(rmObj);
    midLay->addWidget(mergeObj);

    connect(bSph, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("sphere")); });
    connect(bBox, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("cube")); });
    connect(bCyl, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("cylinder")); });
    connect(bTor, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("torus")); });
    connect(bMesh, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("mesh")); });
    connect(rmObj, &QPushButton::clicked, this, &MainWindow::onRemoveObject);
    connect(mergeObj, &QPushButton::clicked, this, &MainWindow::onMergeSelected);
    connect(m_objectList, &QListWidget::itemSelectionChanged, this, &MainWindow::onObjectSelectionChanged);
    connect(m_objectList, &QListWidget::itemDoubleClicked, this, &MainWindow::onObjectItemActivated);

    auto* rightScroll = new QScrollArea;
    rightScroll->setWidgetResizable(true);
    auto* rightInner = new QWidget;
    auto* rightVBox = new QVBoxLayout(rightInner);
    auto* toolBox = new QToolBox(rightInner);
    auto* transformPage = new QWidget;
    auto* transformForm = new QFormLayout(transformPage);
    auto* physicsPage = new QWidget;
    auto* physicsForm = new QFormLayout(physicsPage);
    auto* fourdPage = new QWidget;
    auto* fourdForm = new QFormLayout(fourdPage);
    auto* extrasPage = new QWidget;
    auto* extrasForm = new QFormLayout(extrasPage);
    auto* scriptPage = new QWidget;
    auto* scriptForm = new QFormLayout(scriptPage);
    m_typeLabel = new QLabel(QStringLiteral("—"));
    addI18nFormRow(transformForm, "Type", m_typeLabel);

    m_px = new QDoubleSpinBox;
    m_py = new QDoubleSpinBox;
    m_pz = new QDoubleSpinBox;
    m_sx = new QDoubleSpinBox;
    m_sy = new QDoubleSpinBox;
    m_sz = new QDoubleSpinBox;
    m_rx = new QDoubleSpinBox;
    m_ry = new QDoubleSpinBox;
    m_rz = new QDoubleSpinBox;
    m_vx = new QDoubleSpinBox;
    m_vy = new QDoubleSpinBox;
    m_vz = new QDoubleSpinBox;
    m_pk = new QDoubleSpinBox;
    m_vk = new QDoubleSpinBox;
    m_orbitX = new QDoubleSpinBox;
    m_orbitY = new QDoubleSpinBox;
    m_orbitZ = new QDoubleSpinBox;
    m_orbitOmega = new QDoubleSpinBox;
    m_groupId = new QDoubleSpinBox;
    for (auto* s : {m_px, m_py, m_pz})
        setupSpin(s);
    for (auto* s : {m_sx, m_sy, m_sz}) {
        setupSpin(s);
        s->setValue(1);
    }
    for (auto* s : {m_rx, m_ry, m_rz}) {
        setupSpin(s, -360, 360);
    }
    for (auto* s : {m_vx, m_vy, m_vz, m_orbitX, m_orbitY, m_orbitZ, m_orbitOmega})
        setupSpin(s, -1e6, 1e6);
    setupSpin(m_groupId, -1, 100000);
    m_groupId->setDecimals(0);

    addI18nFormRow(transformForm, "Position X", m_px);
    addI18nFormRow(transformForm, "Position Y", m_py);
    addI18nFormRow(transformForm, "Position Z", m_pz);

    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setSingleStep(1);
    m_opacitySlider->setPageStep(5);
    m_opacitySlider->setValue(100);
    m_opacityValue = new QLabel(QStringLiteral("1.00"));
    m_opacityValue->setMinimumWidth(40);
    m_opacityValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* opacityRow = new QWidget;
    auto* opacityLay = new QHBoxLayout(opacityRow);
    opacityLay->setContentsMargins(0, 0, 0, 0);
    opacityLay->setSpacing(8);
    opacityLay->addWidget(m_opacitySlider, 1);
    opacityLay->addWidget(m_opacityValue);
    addI18nFormRow(transformForm, "Opacity", opacityRow);
    markI18nTip(m_opacitySlider, "0.00–1.00 in steps of 0.01");

    addI18nFormRow(transformForm, "Scale X", m_sx);
    addI18nFormRow(transformForm, "Scale Y", m_sy);
    addI18nFormRow(transformForm, "Scale Z", m_sz);
    addI18nFormRow(transformForm, "Rotation X (°)", m_rx);
    addI18nFormRow(transformForm, "Rotation Y (°)", m_ry);
    addI18nFormRow(transformForm, "Rotation Z (°)", m_rz);
    addI18nFormRow(transformForm, "Group id (-1 none)", m_groupId);

    m_useGravity = new QComboBox;
    addI18nComboItem(m_useGravity, "Off", 0);
    addI18nComboItem(m_useGravity, "Primitive (down vector)", 1);
    addI18nComboItem(m_useGravity, "Advanced (attractor)", 2);
    m_useFriction = new QComboBox;
    addI18nComboItem(m_useFriction, "Off", 0);
    addI18nComboItem(m_useFriction, "On", 1);
    m_gx = new QDoubleSpinBox;
    m_gy = new QDoubleSpinBox;
    m_gz = new QDoubleSpinBox;
    m_groundFriction = new QDoubleSpinBox;
    m_restitution = new QDoubleSpinBox;
    m_collide = new QComboBox;
    addI18nComboItem(m_collide, "Off", 0);
    addI18nComboItem(m_collide, "On", 1);
    m_isStatic = new QComboBox;
    addI18nComboItem(m_isStatic, "Dynamic", 0);
    addI18nComboItem(m_isStatic, "Static (immovable)", 1);
    m_mass = new QDoubleSpinBox;
    m_gravTargetX = new QDoubleSpinBox;
    m_gravTargetY = new QDoubleSpinBox;
    m_gravTargetZ = new QDoubleSpinBox;
    m_gravStrength = new QDoubleSpinBox;
    m_gravTargetObject = new QDoubleSpinBox;
    m_collisionSubdiv = new QSlider(Qt::Horizontal);
    m_collisionPolyCount = new QLabel(QStringLiteral("0"));
    for (auto* s : {m_gx, m_gy, m_gz})
        setupSpin(s, -1000, 1000);
    for (auto* s : {m_gravTargetX, m_gravTargetY, m_gravTargetZ})
        setupSpin(s, -10000, 10000);
    setupSpin(m_gravStrength, 0, 1e6);
    setupSpin(m_gravTargetObject, -1, 100000);
    m_gravTargetObject->setDecimals(0);
    setupSpin(m_groundFriction, 0, 50);
    setupSpin(m_restitution, 0, 2);
    setupSpin(m_mass, 0, 1e6);
    m_mass->setDecimals(4);
    m_mass->setMinimum(0);
    m_mass->setValue(0);
    m_restitution->setValue(0.74);
    addI18nFormRow(physicsForm, "Velocity X", m_vx);
    addI18nFormRow(physicsForm, "Velocity Y", m_vy);
    addI18nFormRow(physicsForm, "Velocity Z", m_vz);
    addI18nFormRow(physicsForm, "Gravity mode", m_useGravity);

    m_primitiveGravBox = new QGroupBox;
    markI18n(m_primitiveGravBox, "Primitive gravity (vector)");
    m_primitiveGravBox->setCheckable(true);
    auto* primitiveLay = new QFormLayout(m_primitiveGravBox);
    addI18nFormRow(primitiveLay, "Gravity X", m_gx);
    addI18nFormRow(primitiveLay, "Gravity Y", m_gy);
    addI18nFormRow(primitiveLay, "Gravity Z", m_gz);
    physicsForm->addRow(m_primitiveGravBox);

    m_advancedGravBox = new QGroupBox;
    markI18n(m_advancedGravBox, "Advanced gravity (attractor)");
    m_advancedGravBox->setCheckable(true);
    auto* advancedLay = new QFormLayout(m_advancedGravBox);
    addI18nFormRow(advancedLay, "Attractor X", m_gravTargetX);
    addI18nFormRow(advancedLay, "Attractor Y", m_gravTargetY);
    addI18nFormRow(advancedLay, "Attractor Z", m_gravTargetZ);
    addI18nFormRow(advancedLay, "Attractor strength", m_gravStrength);
    addI18nFormRow(advancedLay, "Attractor object index (-1 none)", m_gravTargetObject);
    physicsForm->addRow(m_advancedGravBox);

    addI18nFormRow(physicsForm, "Use friction", m_useFriction);
    addI18nFormRow(physicsForm, "Ground friction", m_groundFriction);
    addI18nFormRow(physicsForm, "Restitution", m_restitution);
    addI18nFormRow(physicsForm, "Collisions", m_collide);
    addI18nFormRow(physicsForm, "Static body", m_isStatic);
    addI18nFormRow(physicsForm, "Mass (0=auto)", m_mass);
    m_collisionSubdiv->setRange(1, subdivHi());
    m_collisionSubdiv->setValue(4);
    addI18nFormRow(physicsForm, "Collision detail (subdiv)", m_collisionSubdiv);
    addI18nFormRow(physicsForm, "Collision polygons", m_collisionPolyCount);
    addI18nFormRow(physicsForm, "Orbit center X", m_orbitX);
    addI18nFormRow(physicsForm, "Orbit center Y", m_orbitY);
    addI18nFormRow(physicsForm, "Orbit center Z", m_orbitZ);
    addI18nFormRow(physicsForm, "Orbit omega Y (deg/s)", m_orbitOmega);

    setupSpin(m_pk, -100, 100);
    setupSpin(m_vk, -100, 100);
    m_rwx = new QDoubleSpinBox;
    m_rwy = new QDoubleSpinBox;
    m_rwz = new QDoubleSpinBox;
    for (auto* s : {m_rwx, m_rwy, m_rwz})
        setupSpin(s, -360, 360);
    m_kSlice = new QSlider(Qt::Horizontal);
    m_kSlice->setRange(-200, 200);
    m_kSlice->setValue(0);
    m_fourdView = new QComboBox;
    addI18nComboItem(m_fourdView, "Slice (k = const)", 0);
    addI18nComboItem(m_fourdView, "Project (Hollasch)", 1);
    addI18nFormRow(fourdForm, "K position", m_pk);
    addI18nFormRow(fourdForm, "K velocity", m_vk);
    addI18nFormRow(fourdForm, "Rotate XW (°)", m_rwx);
    addI18nFormRow(fourdForm, "Rotate YW (°)", m_rwy);
    addI18nFormRow(fourdForm, "Rotate ZW (°)", m_rwz);
    addI18nFormRow(fourdForm, "K-slice (view)", m_kSlice);
    addI18nFormRow(fourdForm, "4D view", m_fourdView);

    m_texCombo = new QComboBox;
    addI18nFormRow(transformForm, "Texture", m_texCombo);
    m_colorBtn = new QPushButton;
    markI18n(m_colorBtn, "Object color");
    markI18nTip(m_colorBtn, "Used when Texture is None");
    addI18nFormRow(transformForm, "Color (no texture)", m_colorBtn);
    connect(m_colorBtn, &QPushButton::clicked, this, &MainWindow::onPickObjectColor);

    for (int i = 0; i < kMaxExtras; ++i) {
        m_extraLabel[i] = new QLabel;
        m_extraSpin[i] = new QDoubleSpinBox;
        setupSpin(m_extraSpin[i], -1e6, 1e6);
        extrasForm->addRow(m_extraLabel[i], m_extraSpin[i]);
        m_extraLabel[i]->hide();
        m_extraSpin[i]->hide();
    }

    m_meshInfo = new QLabel(QStringLiteral("—"));
    m_editMesh = new QCheckBox;
    markI18n(m_editMesh, "Edit mode (Tab)");
    auto* meshCube = new QPushButton;
    markI18n(meshCube, "Add Cube");
    auto* meshPlane = new QPushButton;
    markI18n(meshPlane, "Add Plane");
    auto* meshExtrude = new QPushButton;
    markI18n(meshExtrude, "Extrude face (E)");
    addI18nFormRow(extrasForm, "Mesh", m_meshInfo);
    extrasForm->addRow(m_editMesh);
    extrasForm->addRow(meshCube);
    extrasForm->addRow(meshPlane);
    extrasForm->addRow(meshExtrude);
    auto* convertMesh = new QPushButton;
    markI18n(convertMesh, "Convert to mesh");
    extrasForm->addRow(convertMesh);
    connect(meshCube, &QPushButton::clicked, this, &MainWindow::onMeshAddCube);
    connect(meshPlane, &QPushButton::clicked, this, &MainWindow::onMeshAddPlane);
    connect(meshExtrude, &QPushButton::clicked, this, &MainWindow::onMeshExtrude);
    connect(convertMesh, &QPushButton::clicked, this, &MainWindow::onConvertToMesh);
    connect(m_editMesh, &QCheckBox::toggled, this, [this](bool on) {
        if (m_preview)
            m_preview->setEditMode(on);
    });

    m_scriptCombo = new QComboBox;
    m_scriptSrcCombo = new QComboBox;
    auto* compileBtn = new QPushButton;
    markI18n(compileBtn, "Compile");
    auto* clearScript = new QPushButton;
    markI18n(clearScript, "Clear script");
    addI18nFormRow(scriptForm, "Add Script", m_scriptCombo);
    addI18nFormRow(scriptForm, "Source .cpp", m_scriptSrcCombo);
    scriptForm->addRow(compileBtn);
    scriptForm->addRow(clearScript);
    connect(compileBtn, &QPushButton::clicked, this, &MainWindow::onCompileScript);
    connect(clearScript, &QPushButton::clicked, this, [this]() {
        if (m_scriptCombo)
            m_scriptCombo->setCurrentIndex(0);
        applyTransformFromUi();
    });

    m_inspectorBox = toolBox;
    toolBox->addItem(transformPage, i18n("Transform"));
    toolBox->addItem(physicsPage, i18n("Rigidbody"));
    toolBox->addItem(extrasPage, i18n("Shape / Mesh"));
    toolBox->addItem(scriptPage, i18n("Script"));
    toolBox->setProperty("i18n0", QStringLiteral("Transform"));
    toolBox->setProperty("i18n1", QStringLiteral("Rigidbody"));
    toolBox->setProperty("i18n2", QStringLiteral("Shape / Mesh"));
    toolBox->setProperty("i18n3", QStringLiteral("Script"));
    fourdPage->setParent(this);
    fourdPage->hide();
    rightVBox->addWidget(toolBox);
    rightVBox->addStretch();
    rightScroll->setWidget(rightInner);
    rightScroll->setMinimumWidth(260);
    rightScroll->setMaximumWidth(480);

    auto* projectPage = new QWidget;
    auto* projectLay = new QVBoxLayout(projectPage);
    projectLay->setContentsMargins(6, 6, 6, 6);
    projectLay->addWidget(makeI18nLabel("Textures"));
    m_textureList = new QListWidget;
    projectLay->addWidget(m_textureList);
    auto* texBtns = new QHBoxLayout;
    auto* addTex = new QPushButton;
    markI18n(addTex, "Add…");
    auto* rmTex = new QPushButton;
    markI18n(rmTex, "Remove");
    auto* rescanTex = new QPushButton;
    markI18n(rescanTex, "Rescan folder");
    texBtns->addWidget(addTex);
    texBtns->addWidget(rmTex);
    texBtns->addWidget(rescanTex);
    projectLay->addLayout(texBtns);
    connect(addTex, &QPushButton::clicked, this, &MainWindow::onAddTexture);
    connect(rmTex, &QPushButton::clicked, this, &MainWindow::onRemoveTexture);
    connect(rescanTex, &QPushButton::clicked, this, &MainWindow::onRescanTextures);

    m_buildLogView = new QPlainTextEdit;
    m_buildLogView->setReadOnly(true);
    markI18nPlaceholder(m_buildLogView, "Console (make output)…");
    m_buildLogView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    auto* bottomTabs = new QTabWidget;
    m_bottomTabs = bottomTabs;
    bottomTabs->addTab(projectPage, i18n("Project"));
    bottomTabs->addTab(m_buildLogView, i18n("Console"));
    bottomTabs->setProperty("i18n0", QStringLiteral("Project"));
    bottomTabs->setProperty("i18n1", QStringLiteral("Console"));
    bottomTabs->setMinimumHeight(120);
    bottomTabs->setMaximumHeight(280);

    m_preview = new PreviewWidget;
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_preview->setMinimumSize(160, 160);
    connect(m_preview, &PreviewWidget::objectPicked, this, &MainWindow::onPreviewObjectPicked);
    connect(m_preview, &PreviewWidget::meshEdited, this, &MainWindow::onMeshEdited);
    connect(m_preview, &PreviewWidget::debugLayerChanged, this, &MainWindow::syncOverlayActions);
    connect(m_preview, &PreviewWidget::transformEdited, this, &MainWindow::onPreviewTransformEdited);
    connect(m_preview, &PreviewWidget::cameraMoved, this, &MainWindow::onPreviewCameraMoved);

    auto* centerSplit = new QSplitter(Qt::Vertical, this);
    centerSplit->setChildrenCollapsible(false);
    centerSplit->addWidget(m_preview);
    centerSplit->addWidget(bottomTabs);
    centerSplit->setStretchFactor(0, 1);
    centerSplit->setStretchFactor(1, 0);
    centerSplit->setSizes({720, 160});

    mainSplit->addWidget(mid);
    mainSplit->addWidget(centerSplit);
    mainSplit->addWidget(rightScroll);
    mainSplit->setStretchFactor(0, 0);
    mainSplit->setStretchFactor(1, 1);
    mainSplit->setStretchFactor(2, 0);
    mainSplit->setSizes({240, 900, 320});

    outerLay->addWidget(mainSplit);
    statusBar()->showMessage(i18n("Hierarchy | Scene | Inspector   —  F11 full screen, View → Maximize"));

    auto connectSpin = [this](QDoubleSpinBox* s) {
        connect(s, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::applyTransformFromUi);
    };
    for (auto* s : {m_px, m_py, m_pz, m_sx, m_sy, m_sz, m_rx, m_ry, m_rz, m_vx, m_vy, m_vz, m_pk, m_vk,
                    m_rwx, m_rwy, m_rwz,
                    m_orbitX, m_orbitY, m_orbitZ, m_orbitOmega, m_groupId, m_gx, m_gy, m_gz,
                    m_gravTargetX, m_gravTargetY, m_gravTargetZ, m_gravStrength, m_gravTargetObject,
                    m_groundFriction, m_restitution, m_mass})
        connectSpin(s);
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int v) {
        if (m_opacityValue)
            m_opacityValue->setText(QString::number(v / 100.0, 'f', 2));
        applyTransformFromUi();
    });
    for (int i = 0; i < kMaxExtras; ++i)
        connect(m_extraSpin[i], qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::applyTransformFromUi);
    connect(m_texCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onTextureIndexChanged);
    connect(m_useGravity, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_blockSignals)
            return;
        const int mode = m_useGravity->itemData(idx).toInt();
        syncGravityPanels(mode);
        applyTransformFromUi();
    });
    connect(m_useFriction, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { applyTransformFromUi(); });
    connect(m_collide, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { applyTransformFromUi(); });
    connect(m_isStatic, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { applyTransformFromUi(); });
    connect(m_collisionSubdiv, &QSlider::valueChanged, this, [this](int) {
        refreshCollisionPolyCount();
        applyTransformFromUi();
    });
    connect(m_kSlice, &QSlider::valueChanged, this, [this](int v) {
        if (m_preview)
            m_preview->setCam4dK(v * 0.05);
    });
    connect(m_fourdView, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (m_preview)
            m_preview->setSliceMode(m_fourdView->itemData(idx).toInt() == 0);
    });
    connect(m_scriptCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_blockSignals)
            applyTransformFromUi();
    });
    refreshScriptCombo();
    syncGravityPanels(0);
    applyEnginePower(engine::powerLevel());
    updateColorButton();
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen())
        showMaximized();
    else
        showFullScreen();
}

void MainWindow::applyEnginePower(int level)
{
    engine::setPowerLevel(level);
    if (m_collisionSubdiv) {
        const int hi = subdivHi();
        const int v = std::clamp(m_collisionSubdiv->value(), 1, hi);
        m_collisionSubdiv->setRange(1, hi);
        m_collisionSubdiv->setValue(v);
    }
    if (m_preview)
        markPreviewDirty();
    refreshCollisionPolyCount();
    statusBar()->showMessage(QStringLiteral("Engine power: %1").arg(QString::fromUtf8(engine::powerLabel())), 4000);
}

void MainWindow::updateColorButton()
{
    if (!m_colorBtn)
        return;
    const bool noTex = !m_texCombo || m_texCombo->currentData().toInt() < 0;
    m_colorBtn->setEnabled(noTex);
    double cr = 0.75, cg = 0.75, cb = 0.75;
    if (m_editingObjectRow >= 0 && m_editingObjectRow < m_data.objects.size()) {
        const SceneObject& o = m_data.objects[m_editingObjectRow];
        cr = o.cr;
        cg = o.cg;
        cb = o.cb;
    }
    const int r = static_cast<int>(std::clamp(cr, 0.0, 1.0) * 255.0);
    const int g = static_cast<int>(std::clamp(cg, 0.0, 1.0) * 255.0);
    const int b = static_cast<int>(std::clamp(cb, 0.0, 1.0) * 255.0);
    const QColor c(r, g, b);
    m_colorBtn->setStyleSheet(QStringLiteral("background:%1; color:%2; min-height:22px;")
                                  .arg(c.name(), (r + g + b) > 400 ? QStringLiteral("#111") : QStringLiteral("#eee")));
}

void MainWindow::onPickObjectColor()
{
    if (m_editingObjectRow < 0 || m_editingObjectRow >= m_data.objects.size())
        return;
    SceneObject& o = m_data.objects[m_editingObjectRow];
    const QColor start(static_cast<int>(o.cr * 255), static_cast<int>(o.cg * 255), static_cast<int>(o.cb * 255));
    const QColor c = QColorDialog::getColor(start, this, QStringLiteral("Object color"));
    if (!c.isValid())
        return;
    o.cr = c.redF();
    o.cg = c.greenF();
    o.cb = c.blueF();
    updateColorButton();
    markPreviewDirty();
}

void MainWindow::onBuildViewer()
{
    if (m_buildProc) {
        QMessageBox::information(this, QStringLiteral("Build"),
                                 QStringLiteral("Build is already running in background."));
        return;
    }
    const QString root = repoRoot();
    const QString innerMk = QDir(root).filePath(QStringLiteral("inner/Makefile"));
    if (!QFileInfo::exists(innerMk)) {
        QMessageBox::warning(this, QStringLiteral("Build"),
                             QStringLiteral("Cannot find inner/Makefile — project root appears wrong.\n\n"
                                            "Resolved root:\n  %1\n\n"
                                            "Set environment DRIVER_TEST_ROOT to your project folder, or run the editor from the repo.")
                                 .arg(root));
        return;
    }

    const QString defaultScene = QDir(root).filePath(QStringLiteral("inner/default.scene"));
    QString saveErr;
    if (!saveSceneFile(defaultScene, m_data, &saveErr)) {
        QMessageBox::warning(this, QStringLiteral("Build"),
                             QStringLiteral("Could not write inner/default.scene before build (viewer loads this file):\n%1")
                                 .arg(saveErr));
        return;
    }

    const QString innerDir = QDir(root).filePath(QStringLiteral("inner"));
    m_buildProc = new QProcess(this);
    m_buildLog.clear();
    if (m_buildLogView)
        m_buildLogView->clear();
    m_buildProc->setWorkingDirectory(root);
    m_buildProc->setProgram(QStringLiteral("/bin/sh"));
    m_buildProc->setArguments({QStringLiteral("-c"),
                               QStringLiteral("make -C \"%1\" clean && make -C \"%1\"").arg(innerDir)});
    m_buildProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_buildProc, &QProcess::readyReadStandardOutput, this, [this]() {
        if (!m_buildProc)
            return;
        const QString chunk = QString::fromLocal8Bit(m_buildProc->readAllStandardOutput());
        if (chunk.isEmpty())
            return;
        m_buildLog += chunk;
        if (m_buildLogView) {
            m_buildLogView->moveCursor(QTextCursor::End);
            m_buildLogView->insertPlainText(chunk);
            if (QScrollBar* sb = m_buildLogView->verticalScrollBar())
                sb->setValue(sb->maximum());
        }
    });
    connect(m_buildProc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, root, defaultScene](int exitCode, QProcess::ExitStatus) {
                const QString viewer = QDir(root).filePath(QStringLiteral("inner/scene_viewer"));
                if (exitCode == 0) {
                    QMessageBox::information(
                        this, QStringLiteral("Build finished"),
                        QStringLiteral("Viewer:\n  %1\n\n"
                                       "Default scene path (saved before build):\n  %2\n\n"
                                       "Shared textures folder:\n  %3/textures/\n\n"
                                       "Run:\n  cd \"%3\" && ./inner/scene_viewer --power %5 -scene inner/default.scene\n\n"
                                       "make output:\n%4")
                            .arg(viewer, defaultScene, root, m_buildLog)
                            .arg(engine::powerLevel()));
                } else {
                    QMessageBox::warning(this, QStringLiteral("Build failed"),
                                         QStringLiteral("Exit code %1\n\n%2").arg(exitCode).arg(m_buildLog));
                }
                m_buildProc->deleteLater();
                m_buildProc = nullptr;
                m_buildLog.clear();
            });
    m_buildProc->start();
    if (!m_buildProc->waitForStarted(1500)) {
        QMessageBox::warning(this, QStringLiteral("Build"), QStringLiteral("Failed to start background build."));
        m_buildProc->deleteLater();
        m_buildProc = nullptr;
        return;
    }
    QMessageBox::information(this, QStringLiteral("Build"),
                             QStringLiteral("Build started in background. You can continue editing."));
}

QString MainWindow::repoRoot() const
{
    if (!m_currentFile.isEmpty()) {
        QDir d = QFileInfo(m_currentFile).absoluteDir();
        if (d.dirName() == QLatin1String("inner")) {
            d.cdUp();
            const QString r = d.absolutePath();
            if (QFileInfo::exists(QDir(r).filePath(QStringLiteral("inner/Makefile"))))
                return r;
        }
        const QString ap = QFileInfo(m_currentFile).absolutePath();
        if (QFileInfo::exists(QDir(ap).filePath(QStringLiteral("inner/Makefile"))))
            return ap;
    }
    return resolveDriverTestRoot();
}

QString MainWindow::toProjectRelative(const QString& absolutePath) const
{
    QDir root(repoRoot());
    return root.relativeFilePath(QFileInfo(absolutePath).canonicalFilePath());
}

void MainWindow::onOpen()
{
    QString path = QFileDialog::getOpenFileName(this, i18n("Open scene"), repoRoot(),
                                                i18n("Scene (*.scene);;All (*)"));
    if (!path.isEmpty())
        loadFile(path);
}

void MainWindow::onSave()
{
    if (m_currentFile.isEmpty())
        onSaveAs();
    else
        saveToFile(m_currentFile);
}

void MainWindow::onSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, i18n("Save scene"), repoRoot() + QStringLiteral("/inner/default.scene"),
                                                i18n("Scene (*.scene)"));
    if (!path.isEmpty())
        saveToFile(path);
}

bool MainWindow::saveToFile(const QString& path)
{
    QString err;
    if (!saveSceneFile(path, m_data, &err)) {
        QMessageBox::warning(this, i18n("Save failed"), err);
        return false;
    }
    m_currentFile = path;
    setWindowTitle(i18n("Scene Editor") + QStringLiteral(" — %1").arg(path));
    syncPreviewRoot();
    markPreviewDirty();
    return true;
}

void MainWindow::loadFile(const QString& path)
{
    QString err;
    if (!loadSceneFile(path, m_data, &err)) {
        QMessageBox::warning(this, i18n("Load failed"), err);
        return;
    }
    m_currentFile = path;
    setWindowTitle(i18n("Scene Editor") + QStringLiteral(" — %1").arg(path));
    syncPreviewRoot();
    ensureCameraObject();
    refreshTextureList();
    refreshObjectList();
    refreshTextureCombo();
    markPreviewDirty();
    if (!m_data.objects.isEmpty())
        selectObjectRow(m_rowToObject.isEmpty() ? 0 : m_rowToObject[0]);
    refreshTexturesFromFolder();
}

void MainWindow::refreshTextureList()
{
    m_blockSignals = true;
    m_textureList->clear();
    for (int i = 0; i < m_data.textures.size(); ++i)
        m_textureList->addItem(QStringLiteral("%1: %2").arg(i).arg(m_data.textures[i]));
    m_blockSignals = false;
}

QString MainWindow::objectListLabel(int objRow) const
{
    if (objRow < 0 || objRow >= m_data.objects.size())
        return {};
    const SceneObject& o = m_data.objects[objRow];
    QString typeName = isCameraObject(o) ? i18n("Camera") : o.type;
    QString label = QStringLiteral("%1: %2").arg(objRow).arg(typeName);
    if (o.groupId >= 0) {
        int leader = objRow;
        for (int i = 0; i < m_data.objects.size(); ++i) {
            if (m_data.objects[i].groupId == o.groupId) {
                leader = i;
                break;
            }
        }
        if (objRow == leader)
            label = QStringLiteral("%1: %2").arg(objRow).arg(typeName);
        else
            label = QStringLiteral("    %1: %2").arg(objRow).arg(typeName);
        label += QStringLiteral(" [grp %1]").arg(o.groupId);
    }
    label += QStringLiteral(" [%1]").arg(defaultExtraSummary(o));
    return label;
}

int MainWindow::visibleRowForObject(int objRow) const
{
    for (int i = 0; i < m_rowToObject.size(); ++i)
        if (m_rowToObject[i] == objRow)
            return i;
    return -1;
}

void MainWindow::selectObjectRow(int objRow)
{
    if (objRow < 0 || objRow >= m_data.objects.size())
        return;
    if (m_editingObjectRow >= 0 && m_editingObjectRow < m_data.objects.size() && m_editingObjectRow != objRow)
        pushUiToObject(m_editingObjectRow);
    m_editingObjectRow = objRow;
    m_preview->setSelectedObject(objRow);
    loadObjectIntoUi(objRow);
    const int vis = visibleRowForObject(objRow);
    if (vis >= 0) {
        m_blockSignals = true;
        m_objectList->setCurrentRow(vis);
        m_blockSignals = false;
    }
}

void MainWindow::updateObjectListRow(int visRow, int objRow)
{
    if (visRow < 0 || visRow >= m_objectList->count() || objRow < 0 || objRow >= m_data.objects.size())
        return;
    if (QListWidgetItem* item = m_objectList->item(visRow))
        item->setText(objectListLabel(objRow));
}

void MainWindow::refreshObjectList()
{
    int keepObj = -1;
    const int cr = m_objectList->currentRow();
    if (cr >= 0 && cr < m_rowToObject.size())
        keepObj = m_rowToObject[cr];

    m_blockSignals = true;
    m_objectList->clear();
    m_rowToObject.clear();
    for (int i = 0; i < m_data.objects.size(); ++i) {
        m_objectList->addItem(objectListLabel(i));
        m_rowToObject.push_back(i);
    }

    if (keepObj >= 0) {
        for (int i = 0; i < m_rowToObject.size(); ++i) {
            if (m_rowToObject[i] == keepObj) {
                m_objectList->setCurrentRow(i);
                break;
            }
        }
    }
    m_blockSignals = false;
}

void MainWindow::refreshTextureCombo()
{
    m_blockSignals = true;
    m_texCombo->clear();
    m_texCombo->addItem(i18n("None (-1)"), -1);
    m_texCombo->setItemData(0, QStringLiteral("None (-1)"), Qt::UserRole + 1);
    for (int i = 0; i < m_data.textures.size(); ++i)
        m_texCombo->addItem(QStringLiteral("%1: %2").arg(i).arg(m_data.textures[i]), i);
    m_blockSignals = false;
}

void MainWindow::setExtraEditorsForType(const QString& type)
{
    for (int i = 0; i < kMaxExtras; ++i) {
        m_extraLabel[i]->hide();
        m_extraSpin[i]->hide();
    }

    auto showN = [&](int n, const std::vector<QString>& labels) {
        for (int i = 0; i < n && i < kMaxExtras; ++i) {
            m_extraLabel[i]->setText(labels[static_cast<size_t>(i)]);
            m_extraSpin[i]->setRange(-1e6, 1e6);
            m_extraLabel[i]->show();
            m_extraSpin[i]->show();
        }
    };

    if (type == QLatin1String("sphere")) {
        showN(1, { i18n("Radius") });
        m_extraSpin[0]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cube") || type == QLatin1String("box")) {
        showN(3, { i18n("Size X"), i18n("Size Y"), i18n("Size Z") });
        for (int i = 0; i < 3; ++i)
            m_extraSpin[i]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("solid_cube")) {
        showN(1, { i18n("Size") });
        m_extraSpin[0]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cylinder")) {
        showN(2, { i18n("Base radius"), i18n("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("torus")) {
        showN(2, { i18n("Inner R"), i18n("Outer R") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cone")) {
        showN(2, { i18n("Radius"), i18n("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("pyramid")) {
        showN(2, { i18n("Base"), i18n("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("tesseract") || type == QLatin1String("hypersphere") ||
               type == QLatin1String("pyramid4d") || type == QLatin1String("16cell")) {
        showN(1, { i18n("Size") });
        m_extraSpin[0]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("camera")) {
        showN(1, { i18n("Distance") });
        m_extraSpin[0]->setRange(2.0, 500.0);
    }
}

static void syncMassEditorForObject(const SceneObject& o, QDoubleSpinBox* massSpin)
{
    const bool complex = isComplexFigureType(o.type.toStdString());
    massSpin->setEnabled(!complex);
    massSpin->setToolTip(complex
                             ? i18n("Multi-part figure: total mass is computed from all collision parts.")
                             : i18n("0 = auto mass from shape geometry."));
    if (complex)
        massSpin->setValue(0.0);
}

static int collisionPolyCountForObject(const SceneObject& o, int subdiv)
{
    std::vector<double> ex;
    ex.reserve(static_cast<size_t>(o.extra.size()));
    for (double v : o.extra)
        ex.push_back(v);
    std::string err;
    based* obj = createSceneObject(o.type.toStdString(), o.px, o.py, o.pz, o.sx, o.sy, o.sz, o.rx, o.ry, o.rz, ex, 0, &err);
    if (!obj)
        return 0;
    if (!o.meshVerts.isEmpty()) {
        std::vector<vec<>> verts;
        std::vector<int> inds;
        const int nv = o.meshVerts.size() / 3;
        for (int i = 0; i < nv; ++i)
            verts.push_back(vec<>(o.meshVerts[i * 3], o.meshVerts[i * 3 + 1], o.meshVerts[i * 3 + 2]));
        for (int idx : o.meshIndices)
            inds.push_back(idx);
        applyEditableMeshData(obj, verts, inds);
    }
    if (!o.tetVerts.isEmpty() && o.tetVerts.size() % 16 == 0) {
        std::vector<double> packed;
        packed.reserve(static_cast<size_t>(o.tetVerts.size()));
        for (double v : o.tetVerts)
            packed.push_back(v);
        std::vector<Tet4> tets;
        if (unpackFourDTets(packed, tets))
            applyFourDTets(obj, tets);
    }
    std::vector<CollTri> tris;
    const bool ok = collision::buildObjectCollisionMesh(obj, tris, std::max(1, subdiv));
    delete obj;
    return ok ? static_cast<int>(tris.size()) : 0;
}

void MainWindow::syncGravityPanels(int mode)
{
    const bool off = (mode == 0);
    const bool primitive = (mode == 1);
    const bool advanced = (mode == 2);
    if (m_primitiveGravBox) {
        m_primitiveGravBox->setChecked(primitive);
        m_primitiveGravBox->setEnabled(!off);
        m_primitiveGravBox->setFlat(off);
    }
    if (m_advancedGravBox) {
        m_advancedGravBox->setChecked(advanced);
        m_advancedGravBox->setEnabled(!off);
        m_advancedGravBox->setFlat(off);
    }
    for (auto* s : {m_gx, m_gy, m_gz})
        if (s)
            s->setEnabled(primitive);
    for (auto* s : {m_gravTargetX, m_gravTargetY, m_gravTargetZ, m_gravStrength, m_gravTargetObject})
        if (s)
            s->setEnabled(advanced);
}

void MainWindow::refreshCollisionPolyCount()
{
    if (!m_collisionPolyCount)
        return;
    int visRow = m_objectList->currentRow();
    if (visRow < 0 || visRow >= m_rowToObject.size()) {
        m_collisionPolyCount->setText(QStringLiteral("0"));
        return;
    }
    const int row = m_rowToObject[visRow];
    if (row < 0 || row >= m_data.objects.size()) {
        m_collisionPolyCount->setText(QStringLiteral("0"));
        return;
    }
    SceneObject tmp = m_data.objects[row];
    tmp.px = m_px->value();
    tmp.py = m_py->value();
    tmp.pz = m_pz->value();
    tmp.sx = m_sx->value();
    tmp.sy = m_sy->value();
    tmp.sz = m_sz->value();
    tmp.rx = m_rx->value();
    tmp.ry = m_ry->value();
    tmp.rz = m_rz->value();
    tmp.collisionSubdiv = std::clamp(m_collisionSubdiv->value(), 1, subdivHi());
    tmp.extra.clear();
    int n = extraCountForType(tmp.type);
    for (int i = 0; i < n && i < kMaxExtras; ++i)
        tmp.extra.append(m_extraSpin[i]->value());
    int total = collisionPolyCountForObject(tmp, tmp.collisionSubdiv);
    if (row >= 0 && row < m_data.objects.size() && m_data.objects[row].groupId >= 0) {
        const int gid = m_data.objects[row].groupId;
        for (int gi = 0; gi < m_data.objects.size(); ++gi) {
            if (gi == row || m_data.objects[gi].groupId != gid)
                continue;
            const SceneObject& go = m_data.objects[gi];
            total += collisionPolyCountForObject(go, tmp.collisionSubdiv);
        }
    }
    m_collisionPolyCount->setText(QString::number(total));
}

void MainWindow::loadObjectIntoUi(int row)
{
    if (row < 0 || row >= m_data.objects.size())
        return;
    m_blockSignals = true;
    const SceneObject& o = m_data.objects[row];
    m_typeLabel->setText(o.type);
    m_px->setValue(o.px);
    m_py->setValue(o.py);
    m_pz->setValue(o.pz);
    m_sx->setValue(o.sx);
    m_sy->setValue(o.sy);
    m_sz->setValue(o.sz);
    m_rx->setValue(o.rx);
    m_ry->setValue(o.ry);
    m_rz->setValue(o.rz);
    m_vx->setValue(o.vx);
    m_vy->setValue(o.vy);
    m_vz->setValue(o.vz);
    m_pk->setValue(o.pk);
    m_vk->setValue(o.vk);
    m_rwx->setValue(o.rwx);
    m_rwy->setValue(o.rwy);
    m_rwz->setValue(o.rwz);
    m_orbitX->setValue(o.orbitX);
    m_orbitY->setValue(o.orbitY);
    m_orbitZ->setValue(o.orbitZ);
    m_orbitOmega->setValue(o.orbitOmegaY);
    m_groupId->setValue(o.groupId);
    m_useGravity->setCurrentIndex(std::max(0, m_useGravity->findData(o.gravityMode)));
    m_useFriction->setCurrentIndex(std::max(0, m_useFriction->findData(o.useFriction ? 1 : 0)));
    m_gx->setValue(o.gravityX);
    m_gy->setValue(o.gravityY);
    m_gz->setValue(o.gravityZ);
    m_gravTargetX->setValue(o.gravTargetX);
    m_gravTargetY->setValue(o.gravTargetY);
    m_gravTargetZ->setValue(o.gravTargetZ);
    m_gravStrength->setValue(o.gravStrength);
    m_gravTargetObject->setValue(o.gravTargetObject);
    syncGravityPanels(o.gravityMode);
    m_groundFriction->setValue(o.groundFriction);
    m_restitution->setValue(o.restitution);
    m_collide->setCurrentIndex(std::max(0, m_collide->findData(o.collide ? 1 : 0)));
    m_isStatic->setCurrentIndex(std::max(0, m_isStatic->findData(o.isStatic ? 1 : 0)));
    const int opTicks = std::clamp(static_cast<int>(std::lround(std::clamp(o.alpha, 0.0, 1.0) * 100.0)), 0, 100);
    m_opacitySlider->setValue(opTicks);
    m_opacityValue->setText(QString::number(opTicks / 100.0, 'f', 2));
    syncMassEditorForObject(o, m_mass);
    if (m_mass->isEnabled())
        m_mass->setValue(o.mass > 1e-9 ? o.mass : 0.0);
    m_collisionSubdiv->setValue(std::clamp(o.collisionSubdiv, 1, subdivHi()));

    const int texCount = m_texCombo->count();
    if (texCount != m_data.textures.size() + 1)
        refreshTextureCombo();
    const int idx = m_texCombo->findData(o.texIndex);
    m_texCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    updateColorButton();

    setExtraEditorsForType(o.type);
    int n = extraCountForType(o.type);
    for (int i = 0; i < kMaxExtras; ++i) {
        if (i < n && i < o.extra.size())
            m_extraSpin[i]->setValue(o.extra[i]);
        else if (i < n)
            m_extraSpin[i]->setValue(1);
    }
    if (m_scriptCombo) {
        int si = m_scriptCombo->findData(o.scriptPath);
        if (si < 0)
            si = 0;
        m_scriptCombo->setCurrentIndex(si);
    }
    if (m_meshInfo) {
        const int nv = o.meshVerts.size() / 3;
        const int nt = o.meshIndices.size() / 3;
        const int ntet = o.tetVerts.size() / 16;
        if (o.type == QLatin1String("mesh"))
            m_meshInfo->setText(i18n("%1 verts, %2 tris").arg(nv).arg(nt));
        else if (fourd::isFourDType(o.type.toStdString()))
            m_meshInfo->setText(ntet > 0 ? i18n("%1 tets (edited)").arg(ntet)
                                         : i18n("4D (Tab = slice edit)"));
        else
            m_meshInfo->setText(i18n("(not a mesh)"));
    }
    refreshCollisionPolyCount();
    m_blockSignals = false;
}

void MainWindow::pushUiToObject(int row)
{
    if (row < 0 || row >= m_data.objects.size())
        return;
    SceneObject& o = m_data.objects[row];
    const double oldPx = o.px, oldPy = o.py, oldPz = o.pz;
    const double oldRx = o.rx, oldRy = o.ry, oldRz = o.rz;
    o.px = m_px->value();
    o.py = m_py->value();
    o.pz = m_pz->value();
    o.sx = m_sx->value();
    o.sy = m_sy->value();
    o.sz = m_sz->value();
    o.rx = m_rx->value();
    o.ry = m_ry->value();
    o.rz = m_rz->value();
    o.vx = m_vx->value();
    o.vy = m_vy->value();
    o.vz = m_vz->value();
    o.pk = m_pk->value();
    o.vk = m_vk->value();
    o.rwx = m_rwx->value();
    o.rwy = m_rwy->value();
    o.rwz = m_rwz->value();
    o.orbitX = m_orbitX->value();
    o.orbitY = m_orbitY->value();
    o.orbitZ = m_orbitZ->value();
    o.orbitOmegaY = m_orbitOmega->value();
    o.groupId = static_cast<int>(m_groupId->value());
    o.gravityMode = m_useGravity->currentData().toInt();
    syncGravityPanels(o.gravityMode);
    o.useFriction = m_useFriction->currentData().toInt();
    o.gravityX = m_gx->value();
    o.gravityY = m_gy->value();
    o.gravityZ = m_gz->value();
    o.gravTargetX = m_gravTargetX->value();
    o.gravTargetY = m_gravTargetY->value();
    o.gravTargetZ = m_gravTargetZ->value();
    o.gravStrength = m_gravStrength->value();
    o.gravTargetObject = static_cast<int>(m_gravTargetObject->value());
    o.groundFriction = m_groundFriction->value();
    o.restitution = m_restitution->value();
    o.collide = m_collide->currentData().toInt();
    o.isStatic = m_isStatic->currentData().toInt();
    o.alpha = m_opacitySlider->value() / 100.0;
    o.mass = isComplexFigureType(o.type.toStdString()) ? 0.0 : m_mass->value();
    o.collisionSubdiv = std::clamp(m_collisionSubdiv->value(), 1, subdivHi());
    if (o.groupId >= 0) {
        for (int i = 0; i < m_data.objects.size(); ++i) {
            if (m_data.objects[i].groupId == o.groupId) {
                m_data.objects[i].collisionSubdiv = o.collisionSubdiv;
                m_data.objects[i].isStatic = o.isStatic;
            }
        }
    }
    o.texIndex = m_texCombo->currentData().toInt();
    if (m_scriptCombo)
        o.scriptPath = m_scriptCombo->currentData().toString();

    o.extra.clear();
    int n = extraCountForType(o.type);
    for (int i = 0; i < n && i < kMaxExtras; ++i)
        o.extra.append(m_extraSpin[i]->value());

    if (isCameraObject(o)) {
        o.collide = 0;
        o.isStatic = 1;
        o.gravityMode = 0;
        if (o.extra.isEmpty())
            o.extra.append(35.0);
        if (m_preview)
            m_preview->applyOrbitFromObject(o);
    }

    if (o.groupId >= 0) {
        const double dpx = o.px - oldPx;
        const double dpy = o.py - oldPy;
        const double dpz = o.pz - oldPz;
        const double drx = o.rx - oldRx;
        const double dry = o.ry - oldRy;
        const double drz = o.rz - oldRz;
        for (int i = 0; i < m_data.objects.size(); ++i) {
            if (i == row)
                continue;
            SceneObject& g = m_data.objects[i];
            if (g.groupId != o.groupId)
                continue;
            g.px += dpx;
            g.py += dpy;
            g.pz += dpz;
            g.rx += drx;
            g.ry += dry;
            g.rz += drz;
        }
    }
    refreshCollisionPolyCount();
}

void MainWindow::applyTransformFromUi()
{
    if (m_blockSignals)
        return;
    int visRow = m_objectList->currentRow();
    if (visRow < 0 || visRow >= m_rowToObject.size())
        return;
    int row = m_rowToObject[visRow];
    pushUiToObject(row);
    updateObjectListRow(visRow, row);
    refreshCollisionPolyCount();
    if (isCameraObject(m_data.objects[row])) {
        if (m_preview)
            m_preview->update();
        return;
    }
    markPreviewDirty();
}

void MainWindow::onTextureIndexChanged(int /*index*/)
{
    applyTransformFromUi();
    updateColorButton();
}

void MainWindow::onObjectSelectionChanged()
{
    if (m_blockSignals)
        return;
    if (m_editingObjectRow >= 0 && m_editingObjectRow < m_data.objects.size())
        pushUiToObject(m_editingObjectRow);
    int visRow = m_objectList->currentRow();
    if (visRow < 0 || visRow >= m_rowToObject.size()) {
        m_editingObjectRow = -1;
        m_preview->setSelectedObject(-1);
        return;
    }
    int row = m_rowToObject[visRow];
    m_editingObjectRow = row;
    m_preview->setSelectedObject(row);
    loadObjectIntoUi(row);
}

void MainWindow::onPreviewObjectPicked(int index)
{
    if (index < 0) {
        m_blockSignals = true;
        m_objectList->clearSelection();
        m_blockSignals = false;
        m_preview->setSelectedObject(-1);
        return;
    }
    if (index >= m_data.objects.size())
        return;
    selectObjectRow(index);
}

void MainWindow::onAddTexture()
{
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Image"), repoRoot(),
                                                  QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp);;All (*)"));
    if (path.isEmpty())
        return;
    m_data.textures.append(toProjectRelative(path));
    refreshTextureList();
    refreshTextureCombo();
    markPreviewDirty();
    if (m_objectList->currentRow() >= 0 && m_objectList->currentRow() < m_rowToObject.size())
        loadObjectIntoUi(m_rowToObject[m_objectList->currentRow()]);
}

void MainWindow::onRemoveTexture()
{
    int i = m_textureList->currentRow();
    if (i < 0)
        return;
    m_data.textures.removeAt(i);
    for (SceneObject& o : m_data.objects) {
        if (o.texIndex == i)
            o.texIndex = -1;
        else if (o.texIndex > i)
            o.texIndex--;
    }
    refreshTextureList();
    refreshTextureCombo();
    markPreviewDirty();
    if (m_objectList->currentRow() >= 0 && m_objectList->currentRow() < m_rowToObject.size())
        loadObjectIntoUi(m_rowToObject[m_objectList->currentRow()]);
}

static SceneObject makeObj(const QString& type)
{
    SceneObject o;
    o.type = type;
    o.px = 0;
    o.py = 2;
    o.pz = 0;
    o.sx = o.sy = o.sz = 1;
    o.texIndex = -1;
    o.gravityMode = 0;
    o.useFriction = 0;
    o.gravityY = -9.81;
    o.restitution = 0.12;
    o.collide = 1;
    o.alpha = 1.0;
    o.mass = 0.0;
    o.gravStrength = 120.0;
    o.gravTargetObject = -1;
    o.collisionSubdiv = 4;
    if (type == QLatin1String("sphere"))
        o.extra = {1.0};
    else if (type == QLatin1String("cube") || type == QLatin1String("box"))
        o.extra = {1, 1, 1};
    else if (type == QLatin1String("cylinder"))
        o.extra = {0.5, 1.0};
    else if (type == QLatin1String("torus"))
        o.extra = {0.35, 2.2};
    else if (type == QLatin1String("solid_cube"))
        o.extra = {1.0};
    else if (type == QLatin1String("cone"))
        o.extra = {0.5, 1.0};
    else if (type == QLatin1String("pyramid"))
        o.extra = {1.0, 1.2};
    else if (type == QLatin1String("tesseract") || type == QLatin1String("hypersphere") ||
             type == QLatin1String("pyramid4d") || type == QLatin1String("16cell"))
        o.extra = {1.0};
    else if (type == QLatin1String("mesh")) {
        o.extra.clear();
        o.meshVerts = {-0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5,
                       -0.5, -0.5, 0.5,  0.5, -0.5, 0.5,  0.5, 0.5, 0.5,  -0.5, 0.5, 0.5};
        o.meshIndices = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 4, 5, 0, 5, 1,
                         3, 2, 6, 3, 6, 7, 0, 3, 7, 0, 7, 4, 1, 5, 6, 1, 6, 2};
    } else if (type == QLatin1String("camera")) {
        o.py = 2;
        o.collide = 0;
        o.isStatic = 1;
        o.gravityMode = 0;
        o.extra = {35.0};
    }
    return o;
}

void MainWindow::addFigureFromPreset(const CustomFigurePreset& preset)
{
    m_data.objects.append(preset.object);
    refreshObjectList();
    selectObjectRow(m_data.objects.size() - 1);
    markPreviewDirty();
}

void MainWindow::addFigure(const QString& type)
{
    CustomFigurePreset p;
    p.name = type;
    p.object = makeObj(type);
    addFigureFromPreset(p);
}

void MainWindow::refreshCustomFigureButtons()
{
    if (!m_customFiguresLayout)
        return;
    while (QLayoutItem* item = m_customFiguresLayout->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }

    if (m_customFigures.isEmpty()) {
        auto* hint = new QLabel(QStringLiteral("(none — use “Save selected as custom…”)"));
        hint->setWordWrap(true);
        m_customFiguresLayout->addWidget(hint);
        return;
    }

    auto* gridHost = new QWidget;
    auto* grid = new QGridLayout(gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < m_customFigures.size(); ++i) {
        const CustomFigurePreset preset = m_customFigures[i];
        auto* btn = new QPushButton(preset.name);
        btn->setAutoDefault(false);
        btn->setDefault(false);
        connect(btn, &QPushButton::clicked, this, [this, preset]() { addFigureFromPreset(preset); });
        grid->addWidget(btn, i / 2, i % 2);
    }
    m_customFiguresLayout->addWidget(gridHost);
}

void MainWindow::onSaveCustomFigure()
{
    const int visRow = m_objectList->currentRow();
    if (visRow < 0 || visRow >= m_rowToObject.size()) {
        QMessageBox::information(this, QStringLiteral("Custom figure"),
                               QStringLiteral("Select an object in the list first."));
        return;
    }
    const int row = m_rowToObject[visRow];
    if (isCameraObject(m_data.objects[row]))
        return;
    pushUiToObject(row);

    bool ok = false;
    QString name = QInputDialog::getText(this, QStringLiteral("Save custom figure"),
                                           QStringLiteral("Preset name:"), QLineEdit::Normal,
                                           m_data.objects[row].type, &ok);
    name = name.trimmed();
    if (!ok || name.isEmpty())
        return;

    CustomFigurePreset preset;
    preset.name = name;
    preset.object = m_data.objects[row];

    for (int i = 0; i < m_customFigures.size(); ++i) {
        if (m_customFigures[i].name == name) {
            if (QMessageBox::question(this, QStringLiteral("Overwrite?"),
                                      QStringLiteral("Replace existing preset “%1”?").arg(name))
                != QMessageBox::Yes)
                return;
            m_customFigures[i] = preset;
            QString err;
            if (!saveCustomFiguresCatalog(m_customCatalogPath, m_customFigures, &err)) {
                QMessageBox::warning(this, i18n("Save failed"), err);
                return;
            }
            refreshCustomFigureButtons();
            QMessageBox::information(this, QStringLiteral("Saved"),
                                     QStringLiteral("Updated preset “%1” in\n%2").arg(name, m_customCatalogPath));
            return;
        }
    }

    m_customFigures.append(preset);
    QString err;
    if (!saveCustomFiguresCatalog(m_customCatalogPath, m_customFigures, &err)) {
        m_customFigures.removeLast();
        QMessageBox::warning(this, i18n("Save failed"), err);
        return;
    }
    refreshCustomFigureButtons();
    QMessageBox::information(this, QStringLiteral("Saved"),
                             QStringLiteral("Added preset “%1” to\n%2").arg(name, m_customCatalogPath));
}

void MainWindow::onRemoveObject()
{
    const QModelIndexList sel = m_objectList->selectionModel()->selectedIndexes();
    if (sel.isEmpty())
        return;
    QVector<int> rows;
    rows.reserve(sel.size());
    for (const QModelIndex& idx : sel)
        if (idx.row() >= 0 && idx.row() < m_rowToObject.size())
            rows.push_back(m_rowToObject[idx.row()]);
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    for (int r : rows) {
        if (r >= 0 && r < m_data.objects.size() && !isCameraObject(m_data.objects[r]))
            m_data.objects.removeAt(r);
    }
    clampSceneTextureIndices(m_data);
    refreshObjectList();
    if (!m_data.objects.isEmpty())
        selectObjectRow(m_rowToObject[0]);
    else {
        m_editingObjectRow = -1;
        m_preview->setSelectedObject(-1);
    }
    markPreviewDirty();
}

void MainWindow::onMergeSelected()
{
    const QModelIndexList sel = m_objectList->selectionModel()->selectedIndexes();
    if (sel.size() < 2)
        return;
    int newGroupId = 0;
    for (const SceneObject& o : m_data.objects)
        newGroupId = std::max(newGroupId, o.groupId + 1);
    for (const QModelIndex& idx : sel) {
        if (idx.row() >= 0 && idx.row() < m_rowToObject.size()) {
            int oi = m_rowToObject[idx.row()];
            if (oi >= 0 && oi < m_data.objects.size() && !isCameraObject(m_data.objects[oi]))
                m_data.objects[oi].groupId = newGroupId;
        }
    }
    refreshObjectList();
    markPreviewDirty();
}

void MainWindow::onObjectItemActivated(QListWidgetItem* item)
{
    if (!item)
        return;
    const int visRow = m_objectList->row(item);
    if (visRow < 0 || visRow >= m_rowToObject.size())
        return;
    const int row = m_rowToObject[visRow];
    if (row < 0 || row >= m_data.objects.size())
        return;
    const int gid = m_data.objects[row].groupId;
    if (gid < 0)
        return;
    QStringList details;
    QList<int> rows;
    for (int i = 0; i < m_data.objects.size(); ++i) {
        if (m_data.objects[i].groupId == gid) {
            details << QStringLiteral("#%1 %2").arg(i).arg(m_data.objects[i].type);
            rows.push_back(i);
        }
    }
    bool ok = false;
    QString chosen = QInputDialog::getItem(this, QStringLiteral("Merged group"),
                                           QStringLiteral("Group %1 members:").arg(gid),
                                           details, 0, false, &ok);
    if (!ok || chosen.isEmpty())
        return;
    selectObjectRow(rows.at(details.indexOf(chosen)));
}

void MainWindow::refreshTexturesFromFolder()
{
    const QStringList oldTextures = m_data.textures;
    const QStringList fromDisk = scanRepoTexturesFolder(repoRoot());

    QStringList merged = oldTextures;
    for (const QString& p : fromDisk) {
        if (!merged.contains(p))
            merged.append(p);
    }
    m_data.textures = merged;
    remapSceneTextureIndicesByPath(m_data, oldTextures);
    clampSceneTextureIndices(m_data);
    refreshTextureList();
    refreshTextureCombo();
    if (m_objectList->currentRow() >= 0 && m_objectList->currentRow() < m_rowToObject.size())
        loadObjectIntoUi(m_rowToObject[m_objectList->currentRow()]);
    markPreviewDirty();
}

void MainWindow::onRescanTextures()
{
    refreshTexturesFromFolder();
}

void MainWindow::refreshScriptCombo()
{
    if (!m_scriptCombo || !m_scriptSrcCombo)
        return;
    const QString cur = m_scriptCombo->currentData().toString();
    m_blockSignals = true;
    m_scriptCombo->clear();
    m_scriptSrcCombo->clear();
    addI18nComboItem(m_scriptCombo, "(none)", QString());
    const QDir scriptsDir(QDir(repoRoot()).filePath(QStringLiteral("inner/scripts")));
    const QStringList sos = scriptsDir.entryList(QStringList() << QStringLiteral("*.so"), QDir::Files, QDir::Name);
    for (const QString& so : sos)
        m_scriptCombo->addItem(so, QStringLiteral("scripts/") + so);
    const QDir exDir(scriptsDir.filePath(QStringLiteral("examples")));
    const QStringList cpps = exDir.entryList(QStringList() << QStringLiteral("*.cpp"), QDir::Files, QDir::Name);
    for (const QString& cpp : cpps)
        m_scriptSrcCombo->addItem(cpp, exDir.filePath(cpp));
    const int si = m_scriptCombo->findData(cur);
    m_scriptCombo->setCurrentIndex(si >= 0 ? si : 0);
    m_blockSignals = false;
}

void MainWindow::onPlay()
{
    if (m_editingObjectRow >= 0)
        pushUiToObject(m_editingObjectRow);
    m_preview->startPlay();
}

void MainWindow::onPause()
{
    m_preview->pausePlay();
}

void MainWindow::onStop()
{
    m_preview->stopPlay();
}

void MainWindow::onCompileScript()
{
    if (!m_scriptSrcCombo)
        return;
    const QString cpp = m_scriptSrcCombo->currentData().toString();
    if (cpp.isEmpty()) {
        QMessageBox::information(this, i18n("Compile"), i18n("No .cpp selected."));
        return;
    }
    const QFileInfo fi(cpp);
    const QString dest = QDir(repoRoot()).filePath(QStringLiteral("inner/scripts/") + fi.completeBaseName() +
                                                   QStringLiteral(".so"));
    const QString inc = QDir(repoRoot()).filePath(QStringLiteral("inner/headers"));
    QProcess proc;
    proc.start(QStringLiteral("g++"),
               QStringList() << QStringLiteral("-shared") << QStringLiteral("-fPIC") << QStringLiteral("-O2")
                             << QStringLiteral("-I") + inc << QStringLiteral("-o") << dest << cpp);
    if (!proc.waitForFinished(30000)) {
        QMessageBox::warning(this, i18n("Compile"), i18n("g++ timed out."));
        return;
    }
    const QString err = QString::fromLocal8Bit(proc.readAllStandardError());
    if (proc.exitCode() != 0) {
        QMessageBox::warning(this, i18n("Compile failed"), err);
        return;
    }
    refreshScriptCombo();
    const int idx = m_scriptCombo->findData(QStringLiteral("scripts/") + fi.completeBaseName() + QStringLiteral(".so"));
    if (idx >= 0)
        m_scriptCombo->setCurrentIndex(idx);
    applyTransformFromUi();
}

void MainWindow::onMeshAddCube()
{
    m_preview->meshAddCube();
}

void MainWindow::onMeshAddPlane()
{
    m_preview->meshAddPlane();
}

void MainWindow::onMeshExtrude()
{
    m_preview->meshExtrudeSelected();
}

void MainWindow::onConvertToMesh()
{
    if (m_editingObjectRow >= 0 && m_editingObjectRow < m_data.objects.size())
        pushUiToObject(m_editingObjectRow);
    if (!m_preview->convertSelectedToMesh()) {
        QMessageBox::information(this, i18n("Convert to mesh"),
                                 i18n("Could not convert (empty collision mesh or cap)."));
        return;
    }
    markPreviewDirty();
    if (m_editingObjectRow >= 0)
        loadObjectIntoUi(m_editingObjectRow);
    refreshObjectList();
    if (m_editingObjectRow >= 0)
        selectObjectRow(m_editingObjectRow);
}

void MainWindow::onMeshEdited(int index)
{
    if (index == m_editingObjectRow)
        loadObjectIntoUi(index);
}

void MainWindow::onOverlayLayerChosen(int layer)
{
    if (m_preview)
        m_preview->setDebugLayer(layer);
    syncOverlayActions(layer);
}

void MainWindow::syncOverlayActions(int layer)
{
    const int v = ((layer % 3) + 3) % 3;
    m_blockSignals = true;
    if (m_overlayOffAct)
        m_overlayOffAct->setChecked(v == 0);
    if (m_overlayBoundsAct)
        m_overlayBoundsAct->setChecked(v == 1);
    if (m_overlayComAct)
        m_overlayComAct->setChecked(v == 2);
    if (m_overlayCombo) {
        const int idx = m_overlayCombo->findData(v);
        if (idx >= 0)
            m_overlayCombo->setCurrentIndex(idx);
    }
    m_blockSignals = false;
    if (m_preview && m_preview->debugLayer() != v)
        m_preview->setDebugLayer(v);
}

void MainWindow::retranslateUi()
{
    retranslateI18nProperties(this);
    if (m_inspectorBox) {
        for (int i = 0; i < m_inspectorBox->count(); ++i) {
            const QString key = m_inspectorBox->property(QByteArray("i18n") + QByteArray::number(i)).toString();
            if (!key.isEmpty())
                m_inspectorBox->setItemText(i, i18n(key));
        }
    }
    if (m_bottomTabs) {
        for (int i = 0; i < m_bottomTabs->count(); ++i) {
            const QString key = m_bottomTabs->property(QByteArray("i18n") + QByteArray::number(i)).toString();
            if (!key.isEmpty())
                m_bottomTabs->setTabText(i, i18n(key));
        }
    }
    if (m_powerCombo) {
        const int cur = m_powerCombo->currentIndex();
        for (int i = 0; i < m_powerCombo->count(); ++i) {
            const int level = m_powerCombo->itemData(i).toInt();
            const QString key = m_powerCombo->itemData(i, Qt::UserRole + 1).toString();
            if (!key.isEmpty())
                m_powerCombo->setItemText(i, i18n(key));
            else
                m_powerCombo->setItemText(i, powerItemLabel(level));
        }
        m_powerCombo->setCurrentIndex(cur);
    }
    if (m_currentFile.isEmpty())
        setWindowTitle(i18n("Scene Editor"));
    else
        setWindowTitle(i18n("Scene Editor") + QStringLiteral(" — %1").arg(m_currentFile));
    statusBar()->showMessage(i18n("Hierarchy | Scene | Inspector   —  F11 full screen, View → Maximize"));
    if (m_editingObjectRow >= 0 && m_editingObjectRow < m_data.objects.size()) {
        setExtraEditorsForType(m_data.objects[m_editingObjectRow].type);
        loadObjectIntoUi(m_editingObjectRow);
    }
    refreshScriptCombo();
}

bool MainWindow::isCameraObject(const SceneObject& o)
{
    return o.type == QLatin1String("camera");
}

int MainWindow::cameraObjectIndex() const
{
    for (int i = 0; i < m_data.objects.size(); ++i) {
        if (isCameraObject(m_data.objects[i]))
            return i;
    }
    return -1;
}

void MainWindow::ensureCameraObject()
{
    int idx = cameraObjectIndex();
    if (idx < 0) {
        SceneObject cam = makeObj(QStringLiteral("camera"));
        if (m_preview)
            m_preview->writeOrbitToObject(cam);
        m_data.objects.prepend(cam);
        for (int i = 1; i < m_data.objects.size(); ++i) {
            if (m_data.objects[i].gravTargetObject >= 0)
                m_data.objects[i].gravTargetObject += 1;
        }
        idx = 0;
    }
    if (m_preview)
        m_preview->applyOrbitFromObject(m_data.objects[idx]);
}

void MainWindow::onPreviewTransformEdited(int index)
{
    if (index < 0 || index >= m_data.objects.size())
        return;
    if (m_editingObjectRow == index)
        loadObjectIntoUi(index);
    const int vis = visibleRowForObject(index);
    if (vis >= 0)
        updateObjectListRow(vis, index);
}

void MainWindow::onPreviewCameraMoved()
{
    const int idx = cameraObjectIndex();
    if (idx < 0 || !m_preview)
        return;
    m_preview->writeOrbitToObject(m_data.objects[idx]);
    if (m_editingObjectRow == idx)
        loadObjectIntoUi(idx);
}

void MainWindow::onSettings()
{
    QDialog dlg(this);
    dlg.setWindowTitle(i18n("Settings"));
    auto* form = new QFormLayout(&dlg);
    auto* lang = new QComboBox;
    lang->addItem(i18n("English"), QStringLiteral("en"));
    lang->addItem(i18n("Russian"), QStringLiteral("ru"));
    lang->setCurrentIndex(EditorPrefs::instance().language() == EditorLanguage::Russian ? 1 : 0);
    auto* theme = new QComboBox;
    theme->addItem(i18n("Dark"), QStringLiteral("dark"));
    theme->addItem(i18n("Light"), QStringLiteral("light"));
    theme->setCurrentIndex(EditorPrefs::instance().theme() == EditorTheme::Light ? 1 : 0);
    form->addRow(i18n("Language"), lang);
    form->addRow(i18n("Theme"), theme);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return;
    EditorPrefs::instance().setLanguage(EditorPrefs::languageFromCode(lang->currentData().toString()));
    EditorPrefs::instance().setTheme(EditorPrefs::themeFromCode(theme->currentData().toString()));
    applyEditorTheme(EditorPrefs::instance().theme());
    retranslateUi();
}

