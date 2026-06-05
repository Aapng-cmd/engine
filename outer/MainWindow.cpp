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
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

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
    else
        markPreviewDirty();
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
    auto* buildAct = new QAction(QStringLiteral("Build viewer (make)…"), this);
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
    auto* buildBtn = new QPushButton(QStringLiteral("Build viewer (make)"));
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
    texBtns->addWidget(addTex);
    texBtns->addWidget(rmTex);
    leftLay->addLayout(texBtns);
    connect(addTex, &QPushButton::clicked, this, &MainWindow::onAddTexture);
    connect(rmTex, &QPushButton::clicked, this, &MainWindow::onRemoveTexture);

    auto* mid = new QWidget;
    auto* midLay = new QVBoxLayout(mid);
    midLay->addWidget(new QLabel(QStringLiteral("Objects")));
    m_objectList = new QListWidget;
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
    rmObj->setAutoDefault(false);
    midLay->addWidget(rmObj);

    connect(bSph, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("sphere")); });
    connect(bBox, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("box")); });
    connect(bCyl, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("cylinder")); });
    connect(bTor, &QPushButton::clicked, this, [this]() { addFigure(QStringLiteral("torus")); });
    connect(rmObj, &QPushButton::clicked, this, &MainWindow::onRemoveObject);
    connect(m_objectList, &QListWidget::itemSelectionChanged, this, &MainWindow::onObjectSelectionChanged);

    auto* rightScroll = new QScrollArea;
    rightScroll->setWidgetResizable(true);
    auto* rightInner = new QWidget;
    auto* form = new QFormLayout(rightInner);
    m_typeLabel = new QLabel(QStringLiteral("—"));
    form->addRow(QStringLiteral("Type"), m_typeLabel);

    m_px = new QDoubleSpinBox;
    m_py = new QDoubleSpinBox;
    m_pz = new QDoubleSpinBox;
    m_sx = new QDoubleSpinBox;
    m_sy = new QDoubleSpinBox;
    m_sz = new QDoubleSpinBox;
    m_rx = new QDoubleSpinBox;
    m_ry = new QDoubleSpinBox;
    m_rz = new QDoubleSpinBox;
    for (auto* s : {m_px, m_py, m_pz})
        setupSpin(s);
    for (auto* s : {m_sx, m_sy, m_sz}) {
        setupSpin(s);
        s->setValue(1);
    }
    for (auto* s : {m_rx, m_ry, m_rz}) {
        setupSpin(s, -360, 360);
    }

    form->addRow(QStringLiteral("Position X"), m_px);
    form->addRow(QStringLiteral("Position Y"), m_py);
    form->addRow(QStringLiteral("Position Z"), m_pz);
    form->addRow(QStringLiteral("Scale X"), m_sx);
    form->addRow(QStringLiteral("Scale Y"), m_sy);
    form->addRow(QStringLiteral("Scale Z"), m_sz);
    form->addRow(QStringLiteral("Rotation X (°)"), m_rx);
    form->addRow(QStringLiteral("Rotation Y (°)"), m_ry);
    form->addRow(QStringLiteral("Rotation Z (°)"), m_rz);

    m_texCombo = new QComboBox;
    form->addRow(QStringLiteral("Texture"), m_texCombo);

    for (int i = 0; i < kMaxExtras; ++i) {
        m_extraLabel[i] = new QLabel;
        m_extraSpin[i] = new QDoubleSpinBox;
        setupSpin(m_extraSpin[i], -1e6, 1e6);
        form->addRow(m_extraLabel[i], m_extraSpin[i]);
        m_extraLabel[i]->hide();
        m_extraSpin[i]->hide();
    }

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
    for (auto* s : {m_px, m_py, m_pz, m_sx, m_sy, m_sz, m_rx, m_ry, m_rz})
        connectSpin(s);
    for (int i = 0; i < kMaxExtras; ++i)
        connect(m_extraSpin[i], qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindow::applyTransformFromUi);
    connect(m_texCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onTextureIndexChanged);
}

void MainWindow::onBuildViewer()
{
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

    QProcess proc;
    proc.setWorkingDirectory(root);
    proc.setProgram(QStringLiteral("make"));
    proc.setArguments({QStringLiteral("-C"), QDir(root).filePath(QStringLiteral("inner"))});
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();
    if (!proc.waitForFinished(120000)) {
        QMessageBox::warning(this, QStringLiteral("Build"), QStringLiteral("make timed out or failed to start."));
        return;
    }
    const QString out = QString::fromLocal8Bit(proc.readAllStandardOutput());
    if (proc.exitCode() != 0) {
        QMessageBox::warning(this, QStringLiteral("Build failed"),
                             QStringLiteral("Exit code %1\n\n%2").arg(proc.exitCode()).arg(out));
        return;
    }

    const QString viewer = QDir(root).filePath(QStringLiteral("inner/scene_viewer"));
    const QString scene = QDir(root).filePath(QStringLiteral("inner/default.scene"));
    QMessageBox::information(
        this, QStringLiteral("Build finished"),
        QStringLiteral("Viewer:\n  %1\n\n"
                         "Default scene path:\n  %2\n\n"
                         "Shared textures folder:\n  %3/textures/\n\n"
                         "Run:\n  cd \"%3\" && ./inner/scene_viewer -scene inner/default.scene\n\n"
                         "make output:\n%4")
            .arg(viewer, scene, root, out));
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
        loadObjectIntoUi(0);
    }
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
    for (int i = 0; i < m_data.objects.size(); ++i) {
        const SceneObject& o = m_data.objects[i];
        m_objectList->addItem(QStringLiteral("%1: %2  [%3]")
                                  .arg(i)
                                  .arg(o.type)
                                  .arg(defaultExtraSummary(o)));
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
    o.px = m_px->value();
    o.py = m_py->value();
    o.pz = m_pz->value();
    o.sx = m_sx->value();
    o.sy = m_sy->value();
    o.sz = m_sz->value();
    o.rx = m_rx->value();
    o.ry = m_ry->value();
    o.rz = m_rz->value();
    o.texIndex = m_texCombo->currentData().toInt();

    o.extra.clear();
    int n = extraCountForType(o.type);
    for (int i = 0; i < n && i < kMaxExtras; ++i)
        o.extra.append(m_extraSpin[i]->value());
}

void MainWindow::applyTransformFromUi()
{
    if (m_blockSignals)
        return;
    int row = m_objectList->currentRow();
    if (row < 0)
        return;
    pushUiToObject(row);
    m_blockSignals = true;
    m_objectList->item(row)->setText(QStringLiteral("%1: %2  [%3]")
                                         .arg(row)
                                         .arg(m_data.objects[row].type)
                                         .arg(defaultExtraSummary(m_data.objects[row])));
    m_blockSignals = false;
    markPreviewDirty();
}

void MainWindow::onTextureIndexChanged(int /*index*/)
{
    applyTransformFromUi();
}

void MainWindow::onObjectSelectionChanged()
{
    int row = m_objectList->currentRow();
    if (row >= 0)
        loadObjectIntoUi(row);
}

void MainWindow::onPreviewObjectPicked(int index)
{
    if (index < 0 || index >= m_data.objects.size())
        return;
    m_blockSignals = true;
    m_objectList->setCurrentRow(index);
    m_blockSignals = false;
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
    if (m_objectList->currentRow() >= 0)
        loadObjectIntoUi(m_objectList->currentRow());
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
    if (m_objectList->currentRow() >= 0)
        loadObjectIntoUi(m_objectList->currentRow());
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
    const int row = m_data.objects.size() - 1;
    m_objectList->setCurrentRow(row);
    loadObjectIntoUi(row);
    markPreviewDirty();
}

void MainWindow::onRemoveObject()
{
    int row = m_objectList->currentRow();
    if (row < 0)
        return;
    m_data.objects.removeAt(row);
    refreshObjectList();
    markPreviewDirty();
}
