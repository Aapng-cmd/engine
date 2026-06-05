#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct SceneObject {
    QString type;
    double px = 0, py = 0, pz = 0;
    double sx = 1, sy = 1, sz = 1;
    double rx = 0, ry = 0, rz = 0;
    int texIndex = -1;
    QVector<double> extra;
};

struct SceneData {
    QStringList textures;
    QVector<SceneObject> objects;
};

/** Same text format as inner/scene_loader.cpp (VERSION 1). */
bool loadSceneFile(const QString& path, SceneData& out, QString* errorMsg = nullptr);
bool saveSceneFile(const QString& path, const SceneData& data, QString* errorMsg = nullptr);

QString defaultExtraSummary(const SceneObject& o);
