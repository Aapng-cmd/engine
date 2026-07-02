#include "SceneFile.h"
#include "object_factory.h"

#include <QtGlobal>

void clampSceneTextureIndices(SceneData& data)
{
    for (SceneObject& o : data.objects) {
        if (o.texIndex < 0 || o.texIndex >= data.textures.size())
            o.texIndex = -1;
    }
}

void remapSceneTextureIndicesByPath(SceneData& data, const QStringList& oldTextures)
{
    for (SceneObject& o : data.objects) {
        if (o.texIndex < 0 || o.texIndex >= oldTextures.size()) {
            o.texIndex = -1;
            continue;
        }
        const QString path = oldTextures[o.texIndex];
        const int ni = data.textures.indexOf(path);
        o.texIndex = ni >= 0 ? ni : -1;
    }
}
#include <QFile>
#include <QTextStream>
#include <QHash>
#include <algorithm>

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

    struct PhysMeta {
        double vx = 0, vy = 0, vz = 0;
        double ox = 0, oy = 0, oz = 0;
        double omegaY = 0;
        int gravityMode = 0;
        int useFriction = 0;
        double gx = 0, gy = -9.81, gz = 0;
        double tx = 0, ty = 0, tz = 0;
        double gStrength = 120.0;
        int gTargetObj = -1;
        double friction = 0.0;
        double restitution = 0.74;
        int collide = 1;
        double alpha = 1.0;
        double mass = 0.0;
        double pk = 0.0;
        double vk = 0.0;
        int collisionSubdiv = 4;
    };
    QHash<int, PhysMeta> physByIndex;
    QHash<int, int> groupByIndex;

    while (!ts.atEnd()) {
        QString line = trimLine(ts.readLine());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        if (line.startsWith(QLatin1String("ENV"))) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            const QStringList p = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
            const QStringList p = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
            if (p.size() >= 3 && p[1] == QLatin1String("GROUND")) {
                out.env.groundTexture = p[2];
                if (p.size() >= 5) {
                    out.env.groundEdge1 = p[3].toInt();
                    out.env.groundEdge2 = p[4].toInt();
                }
            } else if (p.size() >= 3 && p[1] == QLatin1String("SKY")) {
                out.env.skyTexture = p[2];
                if (p.size() >= 4)
                    out.env.skyRadius = static_cast<unsigned>(p[3].toUInt());
            }
            continue;
        }

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

        if (line.startsWith(QLatin1String("PHYS"))) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            const QStringList p = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
            const QStringList p = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
            if (p.size() < 9) {
                continue;
            }
            bool ok = true;
            const int idx = p[1].toInt(&ok);
            PhysMeta m;
            m.vx = p[2].toDouble(&ok);
            m.vy = p[3].toDouble(&ok);
            m.vz = p[4].toDouble(&ok);
            m.ox = p[5].toDouble(&ok);
            m.oy = p[6].toDouble(&ok);
            m.oz = p[7].toDouble(&ok);
            m.omegaY = p[8].toDouble(&ok);
            if (p.size() >= 16) {
                m.gravityMode = p[9].toInt(&ok);
                m.useFriction = p[10].toInt(&ok);
                m.gx = p[11].toDouble(&ok);
                m.gy = p[12].toDouble(&ok);
                m.gz = p[13].toDouble(&ok);
                m.friction = p[14].toDouble(&ok);
                m.restitution = p[15].toDouble(&ok);
            }
            if (p.size() >= 19) {
                m.collide = p[16].toInt(&ok);
                m.alpha = p[17].toDouble(&ok);
                m.mass = p[18].toDouble(&ok);
            }
            if (p.size() >= 21) {
                m.pk = p[19].toDouble(&ok);
                m.vk = p[20].toDouble(&ok);
            }
            if (p.size() >= 26) {
                m.tx = p[21].toDouble(&ok);
                m.ty = p[22].toDouble(&ok);
                m.tz = p[23].toDouble(&ok);
                m.gStrength = p[24].toDouble(&ok);
                m.gTargetObj = p[25].toInt(&ok);
            }
            if (p.size() >= 27)
                m.collisionSubdiv = p[26].toInt(&ok);
            if (!ok || idx < 0) {
                continue;
            }
            physByIndex[idx] = m;
            continue;
        }

        if (line.startsWith(QLatin1String("GROUP"))) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            const QStringList p = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
            const QStringList p = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
            if (p.size() != 3)
                continue;
            bool ok = true;
            const int idx = p[1].toInt(&ok);
            const int gid = p[2].toInt(&ok);
            if (!ok || idx < 0)
                continue;
            groupByIndex[idx] = gid;
            continue;
        }

        if (!line.startsWith(QLatin1String("OBJECT"))) {
            // permissive for forward/backward compatibility
            continue;
        }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
        const QStringList parts = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
#endif
        if (parts.size() < 12)
            continue;

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

        for (int i = 12; i < parts.size(); ++i)
            o.extra.append(parts[i].toDouble(&ok));

        auto needExtras = [](const QString& t) -> int {
            return expectedExtraCount(t.toStdString());
        };

        int need = needExtras(o.type);
        if (need < 0)
            continue;
        if (o.extra.size() < need) {
            while (o.extra.size() < need)
                o.extra.append(1.0);
        } else if (o.extra.size() > need) {
            o.extra = o.extra.mid(0, need);
        }

        out.objects.append(o);
    }

    for (int i = 0; i < out.objects.size(); ++i) {
        SceneObject& o = out.objects[i];
        if (physByIndex.contains(i)) {
            const PhysMeta& m = physByIndex[i];
            o.vx = m.vx;
            o.vy = m.vy;
            o.vz = m.vz;
            o.orbitX = m.ox;
            o.orbitY = m.oy;
            o.orbitZ = m.oz;
            o.orbitOmegaY = m.omegaY;
            o.gravityMode = m.gravityMode;
            o.useFriction = m.useFriction;
            o.gravityX = m.gx;
            o.gravityY = m.gy;
            o.gravityZ = m.gz;
            o.gravTargetX = m.tx;
            o.gravTargetY = m.ty;
            o.gravTargetZ = m.tz;
            o.gravStrength = m.gStrength;
            o.gravTargetObject = m.gTargetObj;
            o.groundFriction = m.friction;
            o.restitution = m.restitution;
            o.collide = m.collide;
            o.alpha = m.alpha;
            o.mass = m.mass;
            o.pk = m.pk;
            o.vk = m.vk;
            o.collisionSubdiv = qBound(1, m.collisionSubdiv, 24);
        }
        if (groupByIndex.contains(i))
            o.groupId = groupByIndex[i];
    }

    clampSceneTextureIndices(out);
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
    ts << "ENV GROUND " << data.env.groundTexture << " " << data.env.groundEdge1 << " " << data.env.groundEdge2
       << "\n";
    ts << "ENV SKY " << data.env.skyTexture << " " << data.env.skyRadius << "\n";
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

    for (int i = 0; i < data.objects.size(); ++i) {
        const SceneObject& o = data.objects[i];
        ts << "PHYS " << i << " " << fmt(o.vx) << " " << fmt(o.vy) << " " << fmt(o.vz) << " "
           << fmt(o.orbitX) << " " << fmt(o.orbitY) << " " << fmt(o.orbitZ) << " "
           << fmt(o.orbitOmegaY) << " " << o.gravityMode << " " << o.useFriction << " "
           << fmt(o.gravityX) << " " << fmt(o.gravityY) << " " << fmt(o.gravityZ) << " "
           << fmt(o.groundFriction) << " " << fmt(o.restitution) << " "
           << o.collide << " " << fmt(o.alpha) << " "
           << fmt(o.mass) << " " << fmt(o.pk) << " " << fmt(o.vk) << " "
           << fmt(o.gravTargetX) << " " << fmt(o.gravTargetY) << " " << fmt(o.gravTargetZ) << " "
           << fmt(o.gravStrength) << " " << o.gravTargetObject << " "
           << qBound(1, o.collisionSubdiv, 24) << "\n";
        if (o.groupId >= 0)
            ts << "GROUP " << i << " " << o.groupId << "\n";
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
