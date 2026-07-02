#include "MainWindow.h"
#include "object_factory.h"
#include "collision_mesh.h"
#include "CustomFigures.h"
#include "PreviewWidget.h"
#include "ProjectRoot.h"

#include <QAction>
#include <QCoreApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListWidget>
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
#include <QSlider>
#include <QSplitter>
#include <QToolBox>
#include <QVBoxLayout>
#include <algorithm>
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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Scene builder (outer)"));
    resize(1280, 780);
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
    auto* openAct = new QAction(QStringLiteral("Open…"), this);
    auto* saveAct = new QAction(QStringLiteral("Save"), this);
    auto* saveAsAct = new QAction(QStringLiteral("Save as…"), this);
    auto* buildAct = new QAction(QStringLiteral("Build viewer (clean + make)…"), this);
    auto* quitAct = new QAction(QStringLiteral("Quit"), this);
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpen);
    connect(saveAct, &QAction::triggered, this, &MainWindow::onSave);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(buildAct, &QAction::triggered, this, &MainWindow::onBuildViewer);
    connect(quitAct, &QAction::triggered, this, &QWidget::close);

    QMenu* file = menuBar()->addMenu(QStringLiteral("File"));
    file->addAction(openAct);
    file->addAction(saveAct);
    file->addAction(saveAsAct);
    file->addSeparator();
    file->addAction(buildAct);
    file->addSeparator();
    file->addAction(quitAct);

    QMenu* fig = menuBar()->addMenu(QStringLiteral("Figures"));
    const struct {
        const char* name;
        const char* type;
    } figs[] = {
        { "Unit cube", "solid_cube" },
        { "Cone", "cone" },
        { "Pyramid", "pyramid" },
    };
    for (const auto& f : figs) {
        QString t = QString::fromLatin1(f.type);
        auto* a = new QAction(QString::fromUtf8(f.name), this);
        connect(a, &QAction::triggered, this, [this, t]() { addFigure(t); });
        fig->addAction(a);
    }

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outerLay = new QVBoxLayout(central);

    auto* buildRow = new QHBoxLayout;
    auto* buildBtn = new QPushButton(QStringLiteral("Build viewer (clean + make)"));
    buildBtn->setAutoDefault(false);
    buildBtn->setDefault(false);
    connect(buildBtn, &QPushButton::clicked, this, &MainWindow::onBuildViewer);
    buildRow->addWidget(buildBtn);
    buildRow->addStretch();
    outerLay->addLayout(buildRow);

    m_buildLogView = new QPlainTextEdit;
    m_buildLogView->setReadOnly(true);
    m_buildLogView->setMaximumHeight(140);
    m_buildLogView->setPlaceholderText(QStringLiteral("Build log (make output)…"));
    m_buildLogView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    outerLay->addWidget(m_buildLogView);

    auto* vSplit = new QSplitter(Qt::Vertical, this);
    auto* hSplit = new QSplitter(Qt::Horizontal, this);

    auto* left = new QWidget;
    auto* leftLay = new QVBoxLayout(left);
    leftLay->addWidget(new QLabel(QStringLiteral("Textures")));
    m_textureList = new QListWidget;
    leftLay->addWidget(m_textureList);
    auto* texBtns = new QHBoxLayout;
    auto* addTex = new QPushButton(QStringLiteral("Add…"));
    auto* rmTex = new QPushButton(QStringLiteral("Remove"));
    auto* rescanTex = new QPushButton(QStringLiteral("Rescan folder"));
    texBtns->addWidget(addTex);
    texBtns->addWidget(rmTex);
    texBtns->addWidget(rescanTex);
    leftLay->addLayout(texBtns);
    connect(addTex, &QPushButton::clicked, this, &MainWindow::onAddTexture);
    connect(rmTex, &QPushButton::clicked, this, &MainWindow::onRemoveTexture);
    connect(rescanTex, &QPushButton::clicked, this, &MainWindow::onRescanTextures);

    auto* mid = new QWidget;
    auto* midLay = new QVBoxLayout(mid);
    midLay->addWidget(new QLabel(QStringLiteral("Objects")));
    m_objectList = new QListWidget;
    m_objectList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    midLay->addWidget(m_objectList);

    auto* primBox = new QGroupBox(QStringLiteral("Primitives"));
    auto* primGrid = new QGridLayout(primBox);
    auto* bSph = new QPushButton(QStringLiteral("Sphere"));
    auto* bBox = new QPushButton(QStringLiteral("Cube"));
    auto* bCyl = new QPushButton(QStringLiteral("Cylinder"));
    auto* bTor = new QPushButton(QStringLiteral("Torus"));
    for (auto* b : {bSph, bBox, bCyl, bTor}) {
        b->setAutoDefault(false);
        b->setDefault(false);
    }
    primGrid->addWidget(bSph, 0, 0);
    primGrid->addWidget(bBox, 0, 1);
    primGrid->addWidget(bCyl, 1, 0);
    primGrid->addWidget(bTor, 1, 1);
    midLay->addWidget(primBox);

    auto* figBox = new QGroupBox(QStringLiteral("Figures (basic shapes)"));
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
        auto* fb = new QPushButton(QString::fromUtf8(figRows[i].label));
        fb->setAutoDefault(false);
        fb->setDefault(false);
        const QString tt = QString::fromLatin1(figRows[i].type);
        connect(fb, &QPushButton::clicked, this, [this, tt]() { addFigure(tt); });
        figGrid->addWidget(fb, i / 3, i % 3);
    }
    midLay->addWidget(figBox);

    auto* fourdBox = new QGroupBox(QStringLiteral("4D figures"));
    auto* fourdGrid = new QGridLayout(fourdBox);
    const struct {
        const char* label;
        const char* type;
    } fourdRows[] = {
        { "Tesseract", "tesseract" },
        { "Hypersphere", "hypersphere" },
        { "4D pyramid", "pyramid4d" },
    };
    for (int i = 0; i < 3; ++i) {
        auto* fb = new QPushButton(QString::fromUtf8(fourdRows[i].label));
        fb->setAutoDefault(false);
        fb->setDefault(false);
        const QString tt = QString::fromLatin1(fourdRows[i].type);
        connect(fb, &QPushButton::clicked, this, [this, tt]() { addFigure(tt); });
        fourdGrid->addWidget(fb, 0, i);
    }
    midLay->addWidget(fourdBox);

    auto* customBox = new QGroupBox(QStringLiteral("Custom figures (saved)"));
    m_customFiguresLayout = new QVBoxLayout(customBox);
    midLay->addWidget(customBox);
    auto* saveCustomBtn = new QPushButton(QStringLiteral("Save selected as custom…"));
    saveCustomBtn->setAutoDefault(false);
    saveCustomBtn->setDefault(false);
    connect(saveCustomBtn, &QPushButton::clicked, this, &MainWindow::onSaveCustomFigure);
    midLay->addWidget(saveCustomBtn);

    auto* rmObj = new QPushButton(QStringLiteral("Remove selected"));
    auto* mergeObj = new QPushButton(QStringLiteral("Merge selected"));
    rmObj->setAutoDefault(false);
    mergeObj->setAutoDefault(false);
    midLay->addWidget(rmObj);
    midLay->addWidget(mergeObj);

    connect(bSph, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("sphere")); });
    connect(bBox, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("cube")); });
    connect(bCyl, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("cylinder")); });
    connect(bTor, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("torus")); });
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
    m_typeLabel = new QLabel(QStringLiteral("—"));
    transformForm->addRow(QStringLiteral("Type"), m_typeLabel);

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

    transformForm->addRow(QStringLiteral("Position X"), m_px);
    transformForm->addRow(QStringLiteral("Position Y"), m_py);
    transformForm->addRow(QStringLiteral("Position Z"), m_pz);
    transformForm->addRow(QStringLiteral("Scale X"), m_sx);
    transformForm->addRow(QStringLiteral("Scale Y"), m_sy);
    transformForm->addRow(QStringLiteral("Scale Z"), m_sz);
    transformForm->addRow(QStringLiteral("Rotation X (°)"), m_rx);
    transformForm->addRow(QStringLiteral("Rotation Y (°)"), m_ry);
    transformForm->addRow(QStringLiteral("Rotation Z (°)"), m_rz);
    transformForm->addRow(QStringLiteral("Group id (-1 none)"), m_groupId);

    m_useGravity = new QComboBox;
    m_useGravity->addItem(QStringLiteral("Off"), 0);
    m_useGravity->addItem(QStringLiteral("Primitive (down vector)"), 1);
    m_useGravity->addItem(QStringLiteral("Advanced (attractor)"), 2);
    m_useFriction = new QComboBox;
    m_useFriction->addItem(QStringLiteral("Off"), 0);
    m_useFriction->addItem(QStringLiteral("On"), 1);
    m_gx = new QDoubleSpinBox;
    m_gy = new QDoubleSpinBox;
    m_gz = new QDoubleSpinBox;
    m_groundFriction = new QDoubleSpinBox;
    m_restitution = new QDoubleSpinBox;
    m_collide = new QComboBox;
    m_collide->addItem(QStringLiteral("Off"), 0);
    m_collide->addItem(QStringLiteral("On"), 1);
    m_isStatic = new QComboBox;
    m_isStatic->addItem(QStringLiteral("Dynamic"), 0);
    m_isStatic->addItem(QStringLiteral("Static (immovable)"), 1);
    m_alpha = new QDoubleSpinBox;
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
    setupSpin(m_alpha, 0, 2);
    m_alpha->setSingleStep(0.05);
    m_alpha->setValue(1.0);
    m_alpha->setToolTip(QStringLiteral("0–1: opacity. 1–2: reflection strength (2 = full mirror)."));
    setupSpin(m_mass, 0, 1e6);
    m_mass->setDecimals(4);
    m_mass->setMinimum(0);
    m_mass->setValue(0);
    m_restitution->setValue(0.74);
    physicsForm->addRow(QStringLiteral("Velocity X"), m_vx);
    physicsForm->addRow(QStringLiteral("Velocity Y"), m_vy);
    physicsForm->addRow(QStringLiteral("Velocity Z"), m_vz);
    physicsForm->addRow(QStringLiteral("Gravity mode"), m_useGravity);

    m_primitiveGravBox = new QGroupBox(QStringLiteral("Primitive gravity (vector)"));
    m_primitiveGravBox->setCheckable(true);
    auto* primitiveLay = new QFormLayout(m_primitiveGravBox);
    primitiveLay->addRow(QStringLiteral("Gravity X"), m_gx);
    primitiveLay->addRow(QStringLiteral("Gravity Y"), m_gy);
    primitiveLay->addRow(QStringLiteral("Gravity Z"), m_gz);
    physicsForm->addRow(m_primitiveGravBox);

    m_advancedGravBox = new QGroupBox(QStringLiteral("Advanced gravity (attractor)"));
    m_advancedGravBox->setCheckable(true);
    auto* advancedLay = new QFormLayout(m_advancedGravBox);
    advancedLay->addRow(QStringLiteral("Attractor X"), m_gravTargetX);
    advancedLay->addRow(QStringLiteral("Attractor Y"), m_gravTargetY);
    advancedLay->addRow(QStringLiteral("Attractor Z"), m_gravTargetZ);
    advancedLay->addRow(QStringLiteral("Attractor strength"), m_gravStrength);
    advancedLay->addRow(QStringLiteral("Attractor object index (-1 none)"), m_gravTargetObject);
    physicsForm->addRow(m_advancedGravBox);

    physicsForm->addRow(QStringLiteral("Use friction"), m_useFriction);
    physicsForm->addRow(QStringLiteral("Ground friction"), m_groundFriction);
    physicsForm->addRow(QStringLiteral("Restitution"), m_restitution);
    physicsForm->addRow(QStringLiteral("Collisions"), m_collide);
    physicsForm->addRow(QStringLiteral("Static body"), m_isStatic);
    physicsForm->addRow(QStringLiteral("Opacity / reflect (0–2)"), m_alpha);
    physicsForm->addRow(QStringLiteral("Mass (0=auto)"), m_mass);
    m_collisionSubdiv->setRange(1, 24);
    m_collisionSubdiv->setValue(4);
    physicsForm->addRow(QStringLiteral("Collision detail (subdiv)"), m_collisionSubdiv);
    physicsForm->addRow(QStringLiteral("Collision polygons"), m_collisionPolyCount);
    physicsForm->addRow(QStringLiteral("Orbit center X"), m_orbitX);
    physicsForm->addRow(QStringLiteral("Orbit center Y"), m_orbitY);
    physicsForm->addRow(QStringLiteral("Orbit center Z"), m_orbitZ);
    physicsForm->addRow(QStringLiteral("Orbit omega Y (deg/s)"), m_orbitOmega);

    setupSpin(m_pk, -100, 100);
    setupSpin(m_vk, -100, 100);
    fourdForm->addRow(QStringLiteral("K position"), m_pk);
    fourdForm->addRow(QStringLiteral("K velocity"), m_vk);

    m_texCombo = new QComboBox;
    transformForm->addRow(QStringLiteral("Texture"), m_texCombo);

    for (int i = 0; i < kMaxExtras; ++i) {
        m_extraLabel[i] = new QLabel;
        m_extraSpin[i] = new QDoubleSpinBox;
        setupSpin(m_extraSpin[i], -1e6, 1e6);
        extrasForm->addRow(m_extraLabel[i], m_extraSpin[i]);
        m_extraLabel[i]->hide();
        m_extraSpin[i]->hide();
    }
    toolBox->addItem(transformPage, QStringLiteral("Transform"));
    toolBox->addItem(fourdPage, QStringLiteral("4D Transform"));
    toolBox->addItem(physicsPage, QStringLiteral("Physics"));
    toolBox->addItem(extrasPage, QStringLiteral("Shape params"));
    rightVBox->addWidget(toolBox);
    rightVBox->addStretch();
    rightScroll->setWidget(rightInner);
    hSplit->addWidget(left);
    hSplit->addWidget(mid);
    hSplit->addWidget(rightScroll);
    hSplit->setStretchFactor(0, 1);
    hSplit->setStretchFactor(1, 1);
    hSplit->setStretchFactor(2, 2);

    m_preview = new PreviewWidget;
    m_preview->setMinimumHeight(300);
    connect(m_preview, &PreviewWidget::objectPicked, this, &MainWindow::onPreviewObjectPicked);

    vSplit->addWidget(hSplit);
    vSplit->addWidget(m_preview);
    vSplit->setStretchFactor(0, 2);
    vSplit->setStretchFactor(1, 3);

    outerLay->addWidget(vSplit);

    auto connectSpin = [this](QDoubleSpinBox* s) {
        connect(s, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::applyTransformFromUi);
    };
    for (auto* s : {m_px, m_py, m_pz, m_sx, m_sy, m_sz, m_rx, m_ry, m_rz, m_vx, m_vy, m_vz, m_pk, m_vk,
                    m_orbitX, m_orbitY, m_orbitZ, m_orbitOmega, m_groupId, m_gx, m_gy, m_gz,
                    m_gravTargetX, m_gravTargetY, m_gravTargetZ, m_gravStrength, m_gravTargetObject,
                    m_groundFriction, m_restitution, m_alpha, m_mass})
        connectSpin(s);
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
    syncGravityPanels(0);
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
                                       "Run:\n  cd \"%3\" && ./inner/scene_viewer -scene inner/default.scene\n\n"
                                       "make output:\n%4")
                            .arg(viewer, defaultScene, root, m_buildLog));
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
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Open scene"), repoRoot(),
                                                QStringLiteral("Scene (*.scene);;All (*)"));
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
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save scene"), repoRoot() + QStringLiteral("/inner/default.scene"),
                                                QStringLiteral("Scene (*.scene)"));
    if (!path.isEmpty())
        saveToFile(path);
}

bool MainWindow::saveToFile(const QString& path)
{
    QString err;
    if (!saveSceneFile(path, m_data, &err)) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), err);
        return false;
    }
    m_currentFile = path;
    setWindowTitle(QStringLiteral("Scene builder — %1").arg(path));
    syncPreviewRoot();
    markPreviewDirty();
    return true;
}

void MainWindow::loadFile(const QString& path)
{
    QString err;
    if (!loadSceneFile(path, m_data, &err)) {
        QMessageBox::warning(this, QStringLiteral("Load failed"), err);
        return;
    }
    m_currentFile = path;
    setWindowTitle(QStringLiteral("Scene builder — %1").arg(path));
    syncPreviewRoot();
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
    QString label = QStringLiteral("%1: %2").arg(objRow).arg(o.type);
    if (o.groupId >= 0)
        label += QStringLiteral(" [grp %1]").arg(o.groupId);
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
    m_texCombo->addItem(QStringLiteral("None (-1)"), -1);
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
        showN(1, { QStringLiteral("Radius") });
        m_extraSpin[0]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cube") || type == QLatin1String("box")) {
        showN(3, { QStringLiteral("Size X"), QStringLiteral("Size Y"), QStringLiteral("Size Z") });
        for (int i = 0; i < 3; ++i)
            m_extraSpin[i]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("solid_cube")) {
        showN(1, { QStringLiteral("Size") });
        m_extraSpin[0]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cylinder")) {
        showN(2, { QStringLiteral("Base radius"), QStringLiteral("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("torus")) {
        showN(2, { QStringLiteral("Inner R"), QStringLiteral("Outer R") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cone")) {
        showN(2, { QStringLiteral("Radius"), QStringLiteral("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("pyramid")) {
        showN(2, { QStringLiteral("Base"), QStringLiteral("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("tesseract") || type == QLatin1String("hypersphere") ||
               type == QLatin1String("pyramid4d")) {
        showN(1, { QStringLiteral("Size") });
        m_extraSpin[0]->setRange(0.001, 1e6);
    }
}

static void syncMassEditorForObject(const SceneObject& o, QDoubleSpinBox* massSpin)
{
    const bool complex = isComplexFigureType(o.type.toStdString());
    massSpin->setEnabled(!complex);
    massSpin->setToolTip(complex
                             ? QStringLiteral("Multi-part figure: total mass is computed from all collision parts.")
                             : QStringLiteral("0 = auto mass from shape geometry."));
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
    tmp.collisionSubdiv = std::clamp(m_collisionSubdiv->value(), 1, 24);
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
    m_alpha->setValue(o.alpha);
    syncMassEditorForObject(o, m_mass);
    if (m_mass->isEnabled())
        m_mass->setValue(o.mass > 1e-9 ? o.mass : 0.0);
    m_collisionSubdiv->setValue(std::clamp(o.collisionSubdiv, 1, 24));

    const int texCount = m_texCombo->count();
    if (texCount != m_data.textures.size() + 1)
        refreshTextureCombo();
    const int idx = m_texCombo->findData(o.texIndex);
    m_texCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    setExtraEditorsForType(o.type);
    int n = extraCountForType(o.type);
    for (int i = 0; i < kMaxExtras; ++i) {
        if (i < n && i < o.extra.size())
            m_extraSpin[i]->setValue(o.extra[i]);
        else if (i < n)
            m_extraSpin[i]->setValue(1);
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
    o.alpha = m_alpha->value();
    o.mass = isComplexFigureType(o.type.toStdString()) ? 0.0 : m_mass->value();
    o.collisionSubdiv = std::clamp(m_collisionSubdiv->value(), 1, 24);
    if (o.groupId >= 0) {
        for (int i = 0; i < m_data.objects.size(); ++i) {
            if (m_data.objects[i].groupId == o.groupId) {
                m_data.objects[i].collisionSubdiv = o.collisionSubdiv;
                m_data.objects[i].isStatic = o.isStatic;
            }
        }
    }
    o.texIndex = m_texCombo->currentData().toInt();

    o.extra.clear();
    int n = extraCountForType(o.type);
    for (int i = 0; i < n && i < kMaxExtras; ++i)
        o.extra.append(m_extraSpin[i]->value());

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
    markPreviewDirty();
}

void MainWindow::onTextureIndexChanged(int /*index*/)
{
    applyTransformFromUi();
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
             type == QLatin1String("pyramid4d"))
        o.extra = {1.0};
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
                QMessageBox::warning(this, QStringLiteral("Save failed"), err);
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
        QMessageBox::warning(this, QStringLiteral("Save failed"), err);
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
        if (r >= 0 && r < m_data.objects.size())
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
            if (oi >= 0 && oi < m_data.objects.size())
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
