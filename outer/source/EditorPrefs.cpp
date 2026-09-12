#include "EditorPrefs.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPalette>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QToolBar>
#include <QToolTip>
#include <QVariant>
#include <QWidget>

namespace {

const char* kOrg = "DriverTest";
const char* kApp = "SceneEditor";

QHash<QString, QString> russianMap()
{
    static QHash<QString, QString> m;
    if (!m.isEmpty())
        return m;
    auto add = [&](const char* en, const char* ru) { m.insert(QString::fromUtf8(en), QString::fromUtf8(ru)); };
    add("Scene Editor", "Редактор сцен");
    add("File", "Файл");
    add("Open…", "Открыть…");
    add("Save", "Сохранить");
    add("Save as…", "Сохранить как…");
    add("Build viewer (clean + make)…", "Собрать viewer (clean + make)…");
    add("Quit", "Выход");
    add("GameObject", "Объект");
    add("View", "Вид");
    add("Maximize", "Развернуть");
    add("Full Screen", "Полный экран");
    add("Settings…", "Настройки…");
    add("Settings", "Настройки");
    add("Opacity", "Непрозрачность");
    add("Camera", "Камера");
    add("Distance", "Дистанция");
    add("Drag arrows to move, rings to rotate.", "Стрелки — перенос, кольца — поворот.");
    add("Play", "Play");
    add("Pause", "Пауза");
    add("Stop", "Стоп");
    add(" Power ", " Мощность ");
    add("1 = cheapest, 2 = current PC, 10 = 32GB / RTX 4090-class",
        "1 — самый дешёвый, 2 — текущий ПК, 10 — 32 ГБ / класс RTX 4090");
    add("1 — very low", "1 — очень низко");
    add("2 — current PC", "2 — этот ПК");
    add("5 — mid", "5 — средне");
    add("8 — high", "8 — высоко");
    add("10 — 32GB / RTX 4090", "10 — 32 ГБ / RTX 4090");
    add("Hierarchy", "Иерархия");
    add("Primitives", "Примитивы");
    add("Sphere", "Сфера");
    add("Cube", "Куб");
    add("Cylinder", "Цилиндр");
    add("Torus", "Тор");
    add("Mesh", "Меш");
    add("Figures (basic shapes)", "Фигуры (базовые)");
    add("Unit cube", "Единичный куб");
    add("Cone", "Конус");
    add("Pyramid", "Пирамида");
    add("4D figures", "4D-фигуры");
    add("Tesseract", "Тессеракт");
    add("Hypersphere", "Гиперсфера");
    add("5-cell", "5-ячейка");
    add("16-cell", "16-ячейка");
    add("Custom figures (saved)", "Свои фигуры (сохранённые)");
    add("Save selected as custom…", "Сохранить выбранное как фигуру…");
    add("Remove selected", "Удалить выбранные");
    add("Merge selected", "Объединить выбранные");
    add("Type", "Тип");
    add("Position X", "Позиция X");
    add("Position Y", "Позиция Y");
    add("Position Z", "Позиция Z");
    add("Scale X", "Масштаб X");
    add("Scale Y", "Масштаб Y");
    add("Scale Z", "Масштаб Z");
    add("Rotation X (°)", "Поворот X (°)");
    add("Rotation Y (°)", "Поворот Y (°)");
    add("Rotation Z (°)", "Поворот Z (°)");
    add("Group id (-1 none)", "Group id (−1 нет)");
    add("Off", "Выкл");
    add("On", "Вкл");
    add("Primitive (down vector)", "Примитив (вектор вниз)");
    add("Advanced (attractor)", "Доп. (аттрактор)");
    add("Dynamic", "Динамическое");
    add("Static (immovable)", "Статичное (неподвижное)");
    add("0–1: opacity. 1–2: reflection strength (2 = full mirror).",
        "0–1: непрозрачность. 1–2: сила отражения (2 = зеркало).");
    add("Velocity X", "Скорость X");
    add("Velocity Y", "Скорость Y");
    add("Velocity Z", "Скорость Z");
    add("Gravity mode", "Режим гравитации");
    add("Primitive gravity (vector)", "Примитивная гравитация (вектор)");
    add("Gravity X", "Гравитация X");
    add("Gravity Y", "Гравитация Y");
    add("Gravity Z", "Гравитация Z");
    add("Advanced gravity (attractor)", "Доп. гравитация (аттрактор)");
    add("Attractor X", "Аттрактор X");
    add("Attractor Y", "Аттрактор Y");
    add("Attractor Z", "Аттрактор Z");
    add("Attractor strength", "Сила аттрактора");
    add("Attractor object index (-1 none)", "Индекс объекта-аттрактора (−1 нет)");
    add("Use friction", "Трение");
    add("Ground friction", "Трение с полом");
    add("Restitution", "Упругость");
    add("Collisions", "Коллизии");
    add("Static body", "Статичное тело");
    add("Opacity / reflect (0–2)", "Непрозрачность / отражение (0–2)");
    add("0.00–1.00 in steps of 0.01", "0.00–1.00 с шагом 0.01");
    add("Mass (0=auto)", "Масса (0=авто)");
    add("Collision detail (subdiv)", "Детализация коллизий (subdiv)");
    add("Collision polygons", "Полигоны коллизий");
    add("Orbit center X", "Центр орбиты X");
    add("Orbit center Y", "Центр орбиты Y");
    add("Orbit center Z", "Центр орбиты Z");
    add("Orbit omega Y (deg/s)", "Орбита omega Y (°/с)");
    add("K position", "Позиция K");
    add("K velocity", "Скорость K");
    add("Rotate XW (°)", "Поворот XW (°)");
    add("Rotate YW (°)", "Поворот YW (°)");
    add("Rotate ZW (°)", "Поворот ZW (°)");
    add("K-slice (view)", "K-срез (вид)");
    add("4D view", "Вид 4D");
    add("Slice (k = const)", "Срез (k = const)");
    add("Project (Hollasch)", "Проекция (Hollasch)");
    add("Texture", "Текстура");
    add("Object color", "Цвет объекта");
    add("Used when Texture is None", "Используется, если текстура — Нет");
    add("Color (no texture)", "Цвет (без текстуры)");
    add("Edit mode (Tab)", "Режим правки (Tab)");
    add("Add Cube", "Добавить куб");
    add("Add Plane", "Добавить плоскость");
    add("Extrude face (E)", "Выдавить грань (E)");
    add("Convert to mesh", "Преобразовать в меш");
    add("Add Script", "Добавить скрипт");
    add("Source .cpp", "Исходник .cpp");
    add("Compile", "Скомпилировать");
    add("Clear script", "Снять скрипт");
    add("Transform", "Transform");
    add("4D Transform", "4D Transform");
    add("Rigidbody", "Rigidbody");
    add("Shape / Mesh", "Форма / меш");
    add("Script", "Скрипт");
    add("Textures", "Текстуры");
    add("Add…", "Добавить…");
    add("Remove", "Удалить");
    add("Rescan folder", "Обновить папку");
    add("Console (make output)…", "Консоль (вывод make)…");
    add("Project", "Проект");
    add("Console", "Консоль");
    add("Hierarchy | Scene | Inspector   —  F11 full screen, View → Maximize",
        "Иерархия | Сцена | Инспектор   —  F11 полный экран, Вид → Развернуть");
    add("Open scene", "Открыть сцену");
    add("Scene (*.scene);;All (*)", "Сцена (*.scene);;Все (*)");
    add("Save scene", "Сохранить сцену");
    add("Scene (*.scene)", "Сцена (*.scene)");
    add("Save failed", "Ошибка сохранения");
    add("Load failed", "Ошибка загрузки");
    add("None (-1)", "Нет (−1)");
    add("(none)", "(нет)");
    add("Radius", "Радиус");
    add("Size X", "Размер X");
    add("Size Y", "Размер Y");
    add("Size Z", "Размер Z");
    add("Size", "Размер");
    add("Base radius", "Радиус основания");
    add("Height", "Высота");
    add("Inner R", "Внутр. R");
    add("Outer R", "Внешн. R");
    add("Base", "Основание");
    add("Multi-part figure: total mass is computed from all collision parts.",
        "Составная фигура: масса считается по всем частям коллизии.");
    add("0 = auto mass from shape geometry.", "0 = автомасса по геометрии.");
    add("Overlays", "Оверлеи");
    add("Overlay off", "Оверлеи выкл");
    add("Collision bounds", "Границы коллизий");
    add("Centers of mass & velocity", "Центры масс и скорость");
    add("Same overlays as ';' in scene_viewer (also during Play).",
        "Те же слои, что ';' в scene_viewer (в том числе во время Play).");
    add("Language", "Язык");
    add("English", "English");
    add("Russian", "Русский");
    add("Theme", "Тема");
    add("Dark", "Тёмная");
    add("Light", "Светлая");
    add("No .cpp selected.", "Не выбран .cpp.");
    add("g++ timed out.", "g++ превысил время ожидания.");
    add("Compile failed", "Ошибка компиляции");
    add("Could not convert (empty collision mesh or cap).",
        "Не удалось преобразовать (пустая сетка коллизий или кап).");
    add("(not a mesh)", "(не меш)");
    add("4D (Tab = slice edit)", "4D (Tab — правка среза)");
    add("%1 tets (edited)", "%1 тетраэдров (правка)");
    add("%1 verts, %2 tris", "%1 вершин, %2 треугольников");
    add("Image", "Изображение");
    add("Viewer:\n  %1\n\n", "Viewer:\n  %1\n\n");
    add("None", "Нет");
    add("Overlay", "Оверлей");
    return m;
}

void applyObjectI18n(QObject* o)
{
    if (!o)
        return;
    const QVariant key = o->property("i18n");
    if (key.isValid()) {
        const QString t = i18n(key.toString());
        if (auto* a = qobject_cast<QAction*>(o))
            a->setText(t);
        else if (auto* m = qobject_cast<QMenu*>(o))
            m->setTitle(t);
        else if (auto* b = qobject_cast<QAbstractButton*>(o))
            b->setText(t);
        else if (auto* l = qobject_cast<QLabel*>(o))
            l->setText(t);
        else if (auto* g = qobject_cast<QGroupBox*>(o))
            g->setTitle(t);
        else if (auto* tb = qobject_cast<QToolBar*>(o))
            tb->setWindowTitle(t);
    }
    const QVariant tip = o->property("i18nTip");
    if (tip.isValid()) {
        if (auto* w = qobject_cast<QWidget*>(o))
            w->setToolTip(i18n(tip.toString()));
        else if (auto* a = qobject_cast<QAction*>(o))
            a->setToolTip(i18n(tip.toString()));
    }
    const QVariant ph = o->property("i18nPlaceholder");
    if (ph.isValid()) {
        if (auto* e = qobject_cast<QPlainTextEdit*>(o))
            e->setPlaceholderText(i18n(ph.toString()));
        else if (auto* le = qobject_cast<QLineEdit*>(o))
            le->setPlaceholderText(i18n(ph.toString()));
    }
}

} // namespace

EditorPrefs& EditorPrefs::instance()
{
    static EditorPrefs p;
    return p;
}

void EditorPrefs::load()
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    m_language = languageFromCode(s.value(QStringLiteral("language"), QStringLiteral("en")).toString());
    m_theme = themeFromCode(s.value(QStringLiteral("theme"), QStringLiteral("dark")).toString());
}

void EditorPrefs::save() const
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.setValue(QStringLiteral("language"), languageCode());
    s.setValue(QStringLiteral("theme"), themeCode());
}

void EditorPrefs::setLanguage(EditorLanguage lang)
{
    m_language = lang;
    save();
}

void EditorPrefs::setTheme(EditorTheme theme)
{
    m_theme = theme;
    save();
}

QString EditorPrefs::languageCode() const
{
    return m_language == EditorLanguage::Russian ? QStringLiteral("ru") : QStringLiteral("en");
}

EditorLanguage EditorPrefs::languageFromCode(const QString& code)
{
    return code == QLatin1String("ru") ? EditorLanguage::Russian : EditorLanguage::English;
}

QString EditorPrefs::themeCode() const
{
    return m_theme == EditorTheme::Light ? QStringLiteral("light") : QStringLiteral("dark");
}

EditorTheme EditorPrefs::themeFromCode(const QString& code)
{
    return code == QLatin1String("light") ? EditorTheme::Light : EditorTheme::Dark;
}

QString i18n(const char* en)
{
    if (!en)
        return {};
    return i18n(QString::fromUtf8(en));
}

QString i18n(const QString& en)
{
    if (EditorPrefs::instance().language() != EditorLanguage::Russian)
        return en;
    const auto& m = russianMap();
    const auto it = m.find(en);
    if (it != m.end())
        return it.value();
    return en;
}

void applyEditorTheme(EditorTheme theme)
{
    QApplication* app = qApp;
    if (!app)
        return;
    app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette pal;
    if (theme == EditorTheme::Light) {
        pal = app->style()->standardPalette();
        pal.setColor(QPalette::Window, QColor(245, 245, 245));
        pal.setColor(QPalette::WindowText, QColor(32, 32, 32));
        pal.setColor(QPalette::Base, QColor(255, 255, 255));
        pal.setColor(QPalette::AlternateBase, QColor(235, 235, 235));
        pal.setColor(QPalette::ToolTipBase, QColor(255, 255, 240));
        pal.setColor(QPalette::ToolTipText, QColor(32, 32, 32));
        pal.setColor(QPalette::Text, QColor(32, 32, 32));
        pal.setColor(QPalette::Button, QColor(236, 236, 236));
        pal.setColor(QPalette::ButtonText, QColor(32, 32, 32));
        pal.setColor(QPalette::BrightText, QColor(180, 40, 40));
        pal.setColor(QPalette::Highlight, QColor(62, 105, 160));
        pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        pal.setColor(QPalette::Link, QColor(20, 90, 170));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(140, 140, 140));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(140, 140, 140));
    } else {
        pal.setColor(QPalette::Window, QColor(43, 43, 43));
        pal.setColor(QPalette::WindowText, QColor(220, 220, 220));
        pal.setColor(QPalette::Base, QColor(35, 35, 35));
        pal.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        pal.setColor(QPalette::ToolTipBase, QColor(43, 43, 43));
        pal.setColor(QPalette::ToolTipText, QColor(220, 220, 220));
        pal.setColor(QPalette::Text, QColor(220, 220, 220));
        pal.setColor(QPalette::Button, QColor(60, 60, 60));
        pal.setColor(QPalette::ButtonText, QColor(220, 220, 220));
        pal.setColor(QPalette::BrightText, QColor(255, 90, 90));
        pal.setColor(QPalette::Highlight, QColor(62, 105, 160));
        pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        pal.setColor(QPalette::Link, QColor(90, 160, 220));
        pal.setColor(QPalette::Disabled, QPalette::Text, QColor(128, 128, 128));
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
    }
    app->setPalette(pal);
}

void markI18n(QObject* o, const char* key)
{
    if (!o || !key)
        return;
    o->setProperty("i18n", QString::fromUtf8(key));
    applyObjectI18n(o);
}

void markI18nTip(QWidget* w, const char* key)
{
    if (!w || !key)
        return;
    w->setProperty("i18nTip", QString::fromUtf8(key));
    w->setToolTip(i18n(key));
}

void markI18nPlaceholder(QWidget* w, const char* key)
{
    if (!w || !key)
        return;
    w->setProperty("i18nPlaceholder", QString::fromUtf8(key));
    applyObjectI18n(w);
}

void retranslateI18nProperties(QWidget* root)
{
    if (!root)
        return;
    applyObjectI18n(root);
    for (QAction* a : root->findChildren<QAction*>())
        applyObjectI18n(a);
    for (QWidget* w : root->findChildren<QWidget*>()) {
        applyObjectI18n(w);
        if (auto* c = qobject_cast<QComboBox*>(w))
            retranslateComboI18n(c);
    }
}

QLabel* makeI18nLabel(const char* key, QWidget* parent)
{
    auto* l = new QLabel(parent);
    markI18n(l, key);
    return l;
}

void addI18nFormRow(QFormLayout* form, const char* key, QWidget* field)
{
    if (!form || !field)
        return;
    form->addRow(makeI18nLabel(key), field);
}

void addI18nComboItem(QComboBox* c, const char* key, const QVariant& data)
{
    if (!c || !key)
        return;
    c->addItem(i18n(key), data);
    c->setItemData(c->count() - 1, QString::fromUtf8(key), Qt::UserRole + 1);
}

void retranslateComboI18n(QComboBox* c)
{
    if (!c)
        return;
    const int cur = c->currentIndex();
    for (int i = 0; i < c->count(); ++i) {
        const QString k = c->itemData(i, Qt::UserRole + 1).toString();
        if (!k.isEmpty())
            c->setItemText(i, i18n(k));
    }
    if (cur >= 0 && cur < c->count())
        c->setCurrentIndex(cur);
}
