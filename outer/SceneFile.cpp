#include "SceneFile.h"

#include <QtGlobal>
#include <QFile>
#include <QTextStream>

static QString trimLine(const QString& s)
{
    return s.trimmed();
}

bool loadSceneFile(const QString& path, SceneData& out, QString* errorMsg)
{
    out.textures.clear();
    out.objects.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMsg)
            *errorMsg = QStringLiteral("Cannot open: %1").arg(path);
        return false;
    }

    QTextStream ts(&f);
    auto nextLine = [&]() -> QString {
        while (!ts.atEnd()) {
            QString line = trimLine(ts.readLine());
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;
            return line;
        }
        return {};
    };

    QString first = nextLine();
    if (first.isEmpty()) {
        if (errorMsg)
            *errorMsg = QStringLiteral("Empty file");
        return false;
    }
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const QStringList p = first.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
        const QStringList p = first.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
        if (p.size() < 2 || p[0] != QLatin1String("VERSION") || p[1] != QLatin1String("1")) {
            if (errorMsg)
                *errorMsg = QStringLiteral("Expected VERSION 1");
            return false;
        }
    }

    while (!ts.atEnd()) {
        QString line = trimLine(ts.readLine());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        if (line.startsWith(QLatin1String("TEXTURE"))) {
            QString rest = line.mid(QStringLiteral("TEXTURE").length()).trimmed();
            if (rest.isEmpty()) {
                if (errorMsg)
                    *errorMsg = QStringLiteral("TEXTURE without path");
                return false;
            }
            out.textures.append(rest);
            continue;
        }

        if (!line.startsWith(QLatin1String("OBJECT"))) {
            if (errorMsg)
                *errorMsg = QStringLiteral("Unknown line: %1").arg(line);
            return false;
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
        const QStringList parts = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
        if (parts.size() < 12) {
            if (errorMsg)
                *errorMsg = QStringLiteral("Bad OBJECT: %1").arg(line);
            return false;
        }

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
        if (!ok) {
            if (errorMsg)
                *errorMsg = QStringLiteral("Invalid numbers in: %1").arg(line);
            return false;
        }

        for (int i = 12; i < parts.size(); ++i)
            o.extra.append(parts[i].toDouble(&ok));

        auto needExtras = [](const QString& t) -> int {
            if (t == QLatin1String("sphere"))
                return 1;
            if (t == QLatin1String("box"))
                return 3;
            if (t == QLatin1String("cylinder"))
                return 2;
            if (t == QLatin1String("torus"))
                return 2;
            if (t == QLatin1String("planet"))
                return 3;
            if (t == QLatin1String("weirdo"))
                return 4;
            if (t == QLatin1String("param_cylinder"))
                return 5;
            if (t == QLatin1String("fucked_cylinder"))
                return 5;
            if (t == QLatin1String("kabasik"))
                return 3;
            if (t == QLatin1String("tree"))
                return 1;
            if (t == QLatin1String("snowflake"))
                return 1;
            if (t == QLatin1String("kanar"))
                return 3;
            if (t == QLatin1String("snowman"))
                return 1;
            return -1;
        };

        int need = needExtras(o.type);
        if (need < 0) {
            if (errorMsg)
                *errorMsg = QStringLiteral("Unknown type: %1").arg(o.type);
            return false;
        }
        if (o.extra.size() != need) {
            if (errorMsg)
                *errorMsg = QStringLiteral("Type %1 needs %2 extra values, got %3")
                                .arg(o.type)
                                .arg(need)
                                .arg(o.extra.size());
            return false;
        }

        out.objects.append(o);
    }

    return true;
}

bool saveSceneFile(const QString& path, const SceneData& data, QString* errorMsg)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg)
            *errorMsg = QStringLiteral("Cannot write: %1").arg(path);
        return false;
    }

    QTextStream ts(&f);
    ts << "# Scene file (inner viewer + outer editor)\n";
    ts << "VERSION 1\n";
    for (const QString& t : data.textures)
        ts << "TEXTURE " << t << "\n";

    auto fmt = [](double v) -> QString { return QString::number(v, 'g', 12); };

    for (const SceneObject& o : data.objects) {
        ts << "OBJECT " << o.type << " "
           << fmt(o.px) << " " << fmt(o.py) << " " << fmt(o.pz) << " "
           << fmt(o.sx) << " " << fmt(o.sy) << " " << fmt(o.sz) << " "
           << fmt(o.rx) << " " << fmt(o.ry) << " " << fmt(o.rz) << " "
           << o.texIndex;
        for (double e : o.extra)
            ts << " " << fmt(e);
        ts << "\n";
    }

    return true;
}

QString defaultExtraSummary(const SceneObject& o)
{
    QStringList p;
    for (double e : o.extra)
        p << QString::number(e, 'g', 6);
    return p.join(QLatin1Char(' '));
}
