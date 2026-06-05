#include "MainWindow.h"
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
#include <QGroupBox>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStringList>
#include <QInputDialog>
#include <QSet>
#include <QScrollArea>
#include <QSplitter>
#include <QToolBox>
#include <QVBoxLayout>
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
    if (type == QLatin1String("sphere"))
        return 1;
    if (type == QLatin1String("box"))
        return 3;
    if (type == QLatin1String("cylinder"))
        return 2;
    if (type == QLatin1String("torus"))
        return 2;
    if (type == QLatin1String("planet"))
        return 3;
    if (type == QLatin1String("weirdo"))
        return 4;
    if (type == QLatin1String("param_cylinder"))
        return 5;
    if (type == QLatin1String("fucked_cylinder"))
        return 5;
    if (type == QLatin1String("kabasik"))
        return 3;
    if (type == QLatin1String("tree"))
        return 1;
    if (type == QLatin1String("snowflake"))
        return 1;
    if (type == QLatin1String("kanar"))
        return 3;
    if (type == QLatin1String("snowman"))
        return 1;
    return 0;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Scene builder (outer)"));
    resize(1280, 780);
    buildUi();

    m_preview->setSceneData(&m_data);
    syncPreviewRoot();

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
        { "Planet", "planet" },
        { "Weirdo (torus body)", "weirdo" },
        { "Param cylinder", "param_cylinder" },
        { "Fucked cylinder", "fucked_cylinder" },
        { "Kabasik", "kabasik" },
        { "Tree", "tree" },
        { "Snowflake", "snowflake" },
        { "Kanar", "kanar" },
        { "Snowman", "snowman" },
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
    auto* bBox = new QPushButton(QStringLiteral("Box"));
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

    auto* figBox = new QGroupBox(QStringLiteral("Figures (figures.h)"));
    auto* figGrid = new QGridLayout(figBox);
    const struct {
        const char* label;
        const char* type;
    } figRows[] = {
        { "Planet", "planet" },
        { "Weirdo", "weirdo" },
        { "Param cyl.", "param_cylinder" },
        { "Fucked cyl.", "fucked_cylinder" },
        { "Kabasik", "kabasik" },
        { "Tree", "tree" },
        { "Snowflake", "snowflake" },
        { "Kanar", "kanar" },
        { "Snowman", "snowman" },
    };
    for (int i = 0; i < 9; ++i) {
        auto* fb = new QPushButton(QString::fromUtf8(figRows[i].label));
        fb->setAutoDefault(false);
        fb->setDefault(false);
        const QString tt = QString::fromLatin1(figRows[i].type);
        connect(fb, &QPushButton::clicked, this, [this, tt]() { addFigure(tt); });
        figGrid->addWidget(fb, i / 3, i % 3);
    }
    midLay->addWidget(figBox);

    auto* rmObj = new QPushButton(QStringLiteral("Remove selected"));
    auto* mergeObj = new QPushButton(QStringLiteral("Merge selected"));
    rmObj->setAutoDefault(false);
    mergeObj->setAutoDefault(false);
    midLay->addWidget(rmObj);
    midLay->addWidget(mergeObj);

    connect(bSph, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("sphere")); });
    connect(bBox, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("box")); });
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
    m_useGravity->addItem(QStringLiteral("On"), 1);
    m_useFriction = new QComboBox;
    m_useFriction->addItem(QStringLiteral("Off"), 0);
    m_useFriction->addItem(QStringLiteral("On"), 1);
    m_gx = new QDoubleSpinBox;
    m_gy = new QDoubleSpinBox;
    m_gz = new QDoubleSpinBox;
    m_groundFriction = new QDoubleSpinBox;
    m_restitution = new QDoubleSpinBox;
    for (auto* s : {m_gx, m_gy, m_gz})
        setupSpin(s, -1000, 1000);
    setupSpin(m_groundFriction, 0, 50);
    setupSpin(m_restitution, 0, 2);
    m_restitution->setValue(0.74);
    physicsForm->addRow(QStringLiteral("Velocity X"), m_vx);
    physicsForm->addRow(QStringLiteral("Velocity Y"), m_vy);
    physicsForm->addRow(QStringLiteral("Velocity Z"), m_vz);
    physicsForm->addRow(QStringLiteral("Use gravity"), m_useGravity);
    physicsForm->addRow(QStringLiteral("Gravity X"), m_gx);
    physicsForm->addRow(QStringLiteral("Gravity Y"), m_gy);
    physicsForm->addRow(QStringLiteral("Gravity Z"), m_gz);
    physicsForm->addRow(QStringLiteral("Use friction"), m_useFriction);
    physicsForm->addRow(QStringLiteral("Ground friction"), m_groundFriction);
    physicsForm->addRow(QStringLiteral("Restitution"), m_restitution);
    physicsForm->addRow(QStringLiteral("Orbit center X"), m_orbitX);
    physicsForm->addRow(QStringLiteral("Orbit center Y"), m_orbitY);
    physicsForm->addRow(QStringLiteral("Orbit center Z"), m_orbitZ);
    physicsForm->addRow(QStringLiteral("Orbit omega Y (deg/s)"), m_orbitOmega);

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
    for (auto* s : {m_px, m_py, m_pz, m_sx, m_sy, m_sz, m_rx, m_ry, m_rz, m_vx, m_vy, m_vz,
                    m_orbitX, m_orbitY, m_orbitZ, m_orbitOmega, m_groupId, m_gx, m_gy, m_gz,
                    m_groundFriction, m_restitution})
        connectSpin(s);
    for (int i = 0; i < kMaxExtras; ++i)
        connect(m_extraSpin[i], qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::applyTransformFromUi);
    connect(m_texCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onTextureIndexChanged);
    connect(m_useGravity, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { applyTransformFromUi(); });
    connect(m_useFriction, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { applyTransformFromUi(); });
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
    m_buildProc->setWorkingDirectory(root);
    m_buildProc->setProgram(QStringLiteral("/bin/sh"));
    m_buildProc->setArguments({QStringLiteral("-c"),
                               QStringLiteral("make -C \"%1\" clean && make -C \"%1\"").arg(innerDir)});
    m_buildProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_buildProc, &QProcess::readyReadStandardOutput, this, [this]() {
        if (m_buildProc)
            m_buildLog += QString::fromLocal8Bit(m_buildProc->readAllStandardOutput());
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
    if (!m_data.objects.isEmpty()) {
        m_objectList->setCurrentRow(0);
        if (!m_rowToObject.isEmpty())
            loadObjectIntoUi(m_rowToObject[0]);
    }
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

void MainWindow::refreshObjectList()
{
    m_blockSignals = true;
    m_objectList->clear();
    m_rowToObject.clear();
    QSet<int> shownGroups;
    for (int i = 0; i < m_data.objects.size(); ++i) {
        const SceneObject& o = m_data.objects[i];
        if (o.groupId >= 0) {
            if (shownGroups.contains(o.groupId))
                continue;
            shownGroups.insert(o.groupId);
            int members = 0;
            for (const SceneObject& x : m_data.objects)
                if (x.groupId == o.groupId)
                    ++members;
            m_objectList->addItem(QStringLiteral("%1: Group %2 (%3 objects)")
                                      .arg(i)
                                      .arg(o.groupId)
                                      .arg(members));
        } else {
            m_objectList->addItem(QStringLiteral("%1: %2  [G%4] [%3]")
                                      .arg(i)
                                      .arg(o.type)
                                      .arg(defaultExtraSummary(o))
                                      .arg(o.groupId));
        }
        m_rowToObject.push_back(i);
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
    } else if (type == QLatin1String("box")) {
        showN(3, { QStringLiteral("Size X"), QStringLiteral("Size Y"), QStringLiteral("Size Z") });
        for (int i = 0; i < 3; ++i)
            m_extraSpin[i]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("cylinder")) {
        showN(2, { QStringLiteral("Base radius"), QStringLiteral("Height") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("torus")) {
        showN(2, { QStringLiteral("Inner R"), QStringLiteral("Outer R") });
        m_extraSpin[0]->setRange(0.001, 1e6);
        m_extraSpin[1]->setRange(0.001, 1e6);
    } else if (type == QLatin1String("planet")) {
        showN(3, { QStringLiteral("R"), QStringLiteral("alpha_rot"), QStringLiteral("alpha_self_rot") });
    } else if (type == QLatin1String("weirdo")) {
        showN(4, { QStringLiteral("R1"), QStringLiteral("R2"), QStringLiteral("alpha_rot"), QStringLiteral("alpha_self_rot") });
    } else if (type == QLatin1String("param_cylinder")) {
        showN(5, { QStringLiteral("polygons"), QStringLiteral("R"), QStringLiteral("length"), QStringLiteral("alpha_rot"),
                  QStringLiteral("alpha_self_rot") });
        m_extraSpin[0]->setRange(3, 400);
    } else if (type == QLatin1String("fucked_cylinder")) {
        showN(5, { QStringLiteral("polygons"), QStringLiteral("R1"), QStringLiteral("R2"), QStringLiteral("length"),
                  QStringLiteral("alpha_rot") });
        m_extraSpin[0]->setRange(3, 400);
    } else if (type == QLatin1String("kabasik")) {
        showN(3, { QStringLiteral("size"), QStringLiteral("alpha_rot"), QStringLiteral("alpha_self_rot") });
    } else if (type == QLatin1String("tree")) {
        showN(1, { QStringLiteral("length") });
    } else if (type == QLatin1String("snowflake")) {
        showN(1, { QStringLiteral("shades") });
        m_extraSpin[0]->setRange(3, 128);
    } else if (type == QLatin1String("kanar")) {
        showN(3, { QStringLiteral("size"), QStringLiteral("alpha_rot"), QStringLiteral("alpha_self_rot") });
    } else if (type == QLatin1String("snowman")) {
        showN(1, { QStringLiteral("size") });
    }
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
    m_orbitX->setValue(o.orbitX);
    m_orbitY->setValue(o.orbitY);
    m_orbitZ->setValue(o.orbitZ);
    m_orbitOmega->setValue(o.orbitOmegaY);
    m_groupId->setValue(o.groupId);
    m_useGravity->setCurrentIndex(std::max(0, m_useGravity->findData(o.useGravity ? 1 : 0)));
    m_useFriction->setCurrentIndex(std::max(0, m_useFriction->findData(o.useFriction ? 1 : 0)));
    m_gx->setValue(o.gravityX);
    m_gy->setValue(o.gravityY);
    m_gz->setValue(o.gravityZ);
    m_groundFriction->setValue(o.groundFriction);
    m_restitution->setValue(o.restitution);

    refreshTextureCombo();
    int idx = m_texCombo->findData(o.texIndex);
    if (idx >= 0)
        m_texCombo->setCurrentIndex(idx);
    else
        m_texCombo->setCurrentIndex(0);

    setExtraEditorsForType(o.type);
    int n = extraCountForType(o.type);
    for (int i = 0; i < kMaxExtras; ++i) {
        if (i < n && i < o.extra.size())
            m_extraSpin[i]->setValue(o.extra[i]);
        else if (i < n)
            m_extraSpin[i]->setValue(1);
    }
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
    o.orbitX = m_orbitX->value();
    o.orbitY = m_orbitY->value();
    o.orbitZ = m_orbitZ->value();
    o.orbitOmegaY = m_orbitOmega->value();
    o.groupId = static_cast<int>(m_groupId->value());
    o.useGravity = m_useGravity->currentData().toInt();
    o.useFriction = m_useFriction->currentData().toInt();
    o.gravityX = m_gx->value();
    o.gravityY = m_gy->value();
    o.gravityZ = m_gz->value();
    o.groundFriction = m_groundFriction->value();
    o.restitution = m_restitution->value();
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
            g.vx = o.vx;
            g.vy = o.vy;
            g.vz = o.vz;
            g.orbitX = o.orbitX;
            g.orbitY = o.orbitY;
            g.orbitZ = o.orbitZ;
            g.orbitOmegaY = o.orbitOmegaY;
            g.useGravity = o.useGravity;
            g.useFriction = o.useFriction;
            g.gravityX = o.gravityX;
            g.gravityY = o.gravityY;
            g.gravityZ = o.gravityZ;
            g.groundFriction = o.groundFriction;
            g.restitution = o.restitution;
        }
    }
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
    m_blockSignals = true;
    refreshObjectList();
    m_blockSignals = false;
    markPreviewDirty();
}

void MainWindow::onTextureIndexChanged(int /*index*/)
{
    applyTransformFromUi();
}

void MainWindow::onObjectSelectionChanged()
{
    int visRow = m_objectList->currentRow();
    if (visRow < 0 || visRow >= m_rowToObject.size()) {
        m_preview->setSelectedObject(-1);
        return;
    }
    int row = m_rowToObject[visRow];
    m_preview->setSelectedObject(row);
    loadObjectIntoUi(row);
}

void MainWindow::onPreviewObjectPicked(int index)
{
    if (index < 0) {
        m_preview->setSelectedObject(-1);
        return;
    }
    if (index >= m_data.objects.size())
        return;
    m_blockSignals = true;
    int vis = -1;
    for (int i = 0; i < m_rowToObject.size(); ++i) {
        if (m_rowToObject[i] == index) {
            vis = i;
            break;
        }
    }
    if (vis >= 0)
        m_objectList->setCurrentRow(vis);
    m_blockSignals = false;
    m_preview->setSelectedObject(index);
    loadObjectIntoUi(index);
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
    o.useGravity = 0;
    o.useFriction = 0;
    o.gravityY = -9.81;
    o.restitution = 0.74;
    if (type == QLatin1String("sphere"))
        o.extra = {1.0};
    else if (type == QLatin1String("box"))
        o.extra = {1, 1, 1};
    else if (type == QLatin1String("cylinder"))
        o.extra = {0.5, 1.0};
    else if (type == QLatin1String("torus"))
        o.extra = {0.35, 2.2};
    else if (type == QLatin1String("planet"))
        o.extra = {5, 0, 15};
    else if (type == QLatin1String("weirdo"))
        o.extra = {3, 8, 0, 15};
    else if (type == QLatin1String("param_cylinder"))
        o.extra = {30, 1, 10, 0, 0};
    else if (type == QLatin1String("fucked_cylinder"))
        o.extra = {30, 1, 2, 10, 0};
    else if (type == QLatin1String("kabasik"))
        o.extra = {1, 45, 120};
    else if (type == QLatin1String("tree"))
        o.extra = {12};
    else if (type == QLatin1String("snowflake"))
        o.extra = {15};
    else if (type == QLatin1String("kanar"))
        o.extra = {1, 45, 120};
    else if (type == QLatin1String("snowman"))
        o.extra = {1};
    return o;
}

void MainWindow::addFigure(const QString& type)
{
    m_data.objects.append(makeObj(type));
    refreshObjectList();
    const int objRow = m_data.objects.size() - 1;
    int vis = -1;
    for (int i = 0; i < m_rowToObject.size(); ++i)
        if (m_rowToObject[i] == objRow)
            vis = i;
    if (vis >= 0)
        m_objectList->setCurrentRow(vis);
    loadObjectIntoUi(objRow);
    markPreviewDirty();
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
    refreshObjectList();
    if (!m_data.objects.isEmpty())
        m_objectList->setCurrentRow(std::min(0, m_objectList->count() - 1));
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
    int target = rows.at(details.indexOf(chosen));
    m_preview->setSelectedObject(target);
    loadObjectIntoUi(target);
}

void MainWindow::refreshTexturesFromFolder()
{
    const QStringList oldTextures = m_data.textures;
    const QStringList fromDisk = scanRepoTexturesFolder(repoRoot());
    if (fromDisk.isEmpty())
        return;

    QStringList merged = fromDisk;
    for (const QString& p : oldTextures) {
        if (!merged.contains(p))
            merged.append(p);
    }
    m_data.textures = merged;
    for (SceneObject& o : m_data.objects) {
        if (o.texIndex < 0 || o.texIndex >= oldTextures.size()) {
            o.texIndex = -1;
            continue;
        }
        const QString path = oldTextures[o.texIndex];
        const int ni = m_data.textures.indexOf(path);
        o.texIndex = ni >= 0 ? ni : -1;
    }
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
