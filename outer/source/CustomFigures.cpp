#include "CustomFigures.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

static QString trimLine(const QString& s)
{
    return s.trimmed();
}

QString defaultCustomFiguresCatalogPath(const QString& repoRoot)
{
    return QDir(repoRoot).filePath(QStringLiteral("inner/custom_figures.catalog"));
}

static bool parsePhysLine(const QStringList& p, SceneObject& o)
{
    if (p.size() < 9)
        return false;
    bool ok = true;
    o.vx = p[2].toDouble(&ok);
    o.vy = p[3].toDouble(&ok);
    o.vz = p[4].toDouble(&ok);
    o.orbitX = p[5].toDouble(&ok);
    o.orbitY = p[6].toDouble(&ok);
    o.orbitZ = p[7].toDouble(&ok);
    o.orbitOmegaY = p[8].toDouble(&ok);
    if (p.size() >= 16) {
        o.gravityMode = p[9].toInt(&ok);
        o.useFriction = p[10].toInt(&ok);
        o.gravityX = p[11].toDouble(&ok);
        o.gravityY = p[12].toDouble(&ok);
        o.gravityZ = p[13].toDouble(&ok);
        o.groundFriction = p[14].toDouble(&ok);
        o.restitution = p[15].toDouble(&ok);
    }
    if (p.size() >= 19) {
        o.collide = p[16].toInt(&ok);
        o.alpha = p[17].toDouble(&ok);
        o.mass = p[18].toDouble(&ok);
    }
    if (p.size() >= 26) {
        o.gravTargetX = p[21].toDouble(&ok);
        o.gravTargetY = p[22].toDouble(&ok);
        o.gravTargetZ = p[23].toDouble(&ok);
        o.gravStrength = p[24].toDouble(&ok);
        o.gravTargetObject = p[25].toInt(&ok);
    }
    if (p.size() >= 27)
        o.collisionSubdiv = qBound(1, p[26].toInt(&ok), 24);
    return ok;
}

bool loadCustomFiguresCatalog(const QString& path, QVector<CustomFigurePreset>& out, QString* errorMsg)
{
    out.clear();
    QFile f(path);
    if (!f.exists())
        return true;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg)
            *errorMsg = QStringLiteral("Cannot open: %1").arg(path);
        return false;
    }

    QTextStream ts(&f);
    CustomFigurePreset* current = nullptr;

    while (!ts.atEnd()) {
        QString line = trimLine(ts.readLine());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
        const QStringList parts = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
        if (parts.isEmpty())
            continue;

        if (parts[0] == QLatin1String("VERSION")) {
            if (parts.size() < 2 || parts[1] != QLatin1String("1")) {
                if (errorMsg)
                    *errorMsg = QStringLiteral("Expected VERSION 1 in %1").arg(path);
                return false;
            }
            continue;
        }

        if (parts[0] == QLatin1String("PRESET")) {
            if (parts.size() < 2)
                continue;
            CustomFigurePreset preset;
            preset.name = parts[1];
            for (int i = 2; i < parts.size(); ++i)
                preset.name += QLatin1Char(' ') + parts[i];
            out.append(preset);
            current = &out.last();
            continue;
        }

        if (!current)
            continue;

        if (parts[0] == QLatin1String("OBJECT") && parts.size() >= 12) {
            SceneObject o;
            o.type = parts[1];
            bool ok = true;
            o.px = parts[2].toDouble(&ok);
            o.py = parts[3].toDouble(&ok);
            o.pz = parts[4].toDouble(&ok);
            o.sx = parts[5].toDouble(&ok);
            o.sy = parts[6].toDouble(&ok);
            o.sz = parts[7].toDouble(&ok);
            o.rx = parts[8].toDouble(&ok);
            o.ry = parts[9].toDouble(&ok);
            o.rz = parts[10].toDouble(&ok);
            o.texIndex = parts[11].toInt(&ok);
            if (!ok)
                continue;
            o.extra.clear();
            for (int i = 12; i < parts.size(); ++i)
                o.extra.append(parts[i].toDouble(&ok));
            current->object = o;
            continue;
        }

        if (parts[0] == QLatin1String("PHYS"))
            parsePhysLine(parts, current->object);
    }

    return true;
}

bool saveCustomFiguresCatalog(const QString& path, const QVector<CustomFigurePreset>& presets, QString* errorMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg)
            *errorMsg = QStringLiteral("Cannot write: %1").arg(path);
        return false;
    }

    QTextStream ts(&f);
    ts << "# Custom figure presets for scene_editor\n";
    ts << "VERSION 1\n";

    auto fmt = [](double v) -> QString { return QString::number(v, 'g', 12); };

    for (const CustomFigurePreset& preset : presets) {
        const SceneObject& o = preset.object;
        ts << "PRESET " << preset.name << "\n";
        ts << "OBJECT " << o.type << " "
           << fmt(o.px) << " " << fmt(o.py) << " " << fmt(o.pz) << " "
           << fmt(o.sx) << " " << fmt(o.sy) << " " << fmt(o.sz) << " "
           << fmt(o.rx) << " " << fmt(o.ry) << " " << fmt(o.rz) << " "
           << o.texIndex;
        for (double e : o.extra)
            ts << " " << fmt(e);
        ts << "\n";
        ts << "PHYS 0 " << fmt(o.vx) << " " << fmt(o.vy) << " " << fmt(o.vz) << " "
           << fmt(o.orbitX) << " " << fmt(o.orbitY) << " " << fmt(o.orbitZ) << " "
           << fmt(o.orbitOmegaY) << " " << o.gravityMode << " " << o.useFriction << " "
           << fmt(o.gravityX) << " " << fmt(o.gravityY) << " " << fmt(o.gravityZ) << " "
           << fmt(o.groundFriction) << " " << fmt(o.restitution) << " "
           << o.collide << " " << fmt(o.alpha) << " " << fmt(o.mass) << " "
           << fmt(o.gravTargetX) << " " << fmt(o.gravTargetY) << " " << fmt(o.gravTargetZ) << " "
           << fmt(o.gravStrength) << " " << o.gravTargetObject << " "
           << qBound(1, o.collisionSubdiv, 24) << "\n";
    }

    return true;
}
