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
    double vx = 0, vy = 0, vz = 0;
    double orbitX = 0, orbitY = 0, orbitZ = 0;
    double orbitOmegaY = 0;
    int groupId = -1;
    /** 0=off, 1=primitive, 2=advanced(attractor). */
    int gravityMode = 0;
    int useFriction = 0;
    double gravityX = 0, gravityY = -9.81, gravityZ = 0;
    double gravTargetX = 0, gravTargetY = 0, gravTargetZ = 0;
    double gravStrength = 120.0;
    int gravTargetObject = -1;
    double groundFriction = 0.0;
    double restitution = 0.74;
    int collide = 1;
    double alpha = 1.0;
    /** <= 0: auto mass from part geometry; > 0: total mass override. */
    double mass = 0.0;
    /** K-слой (3D фиксирован; 4D — динамика). */
    double pk = 0.0;
    double vk = 0.0;
    /** Collision mesh detail (face subdiv for mesh bodies), 1..24. */
    int collisionSubdiv = 4;
    /** Fully immovable: collides but never moves. */
    int isStatic = 0;
};

struct SceneEnvironment {
    QString groundTexture = QStringLiteral("textures/water.png");
    int groundEdge1 = 200;
    int groundEdge2 = 200;
    QString skyTexture = QStringLiteral("textures/mountains.jpg");
    unsigned skyRadius = 1000;
};

struct SceneData {
    QStringList textures;
    QVector<SceneObject> objects;
    SceneEnvironment env;
};

/** Same text format as inner/scene_loader.cpp (VERSION 1). */
bool loadSceneFile(const QString& path, SceneData& out, QString* errorMsg = nullptr);
bool saveSceneFile(const QString& path, const SceneData& data, QString* errorMsg = nullptr);

QString defaultExtraSummary(const SceneObject& o);

/** Clamp invalid texture indices; remap by path when texture list order changes. */
void clampSceneTextureIndices(SceneData& data);
void remapSceneTextureIndicesByPath(SceneData& data, const QStringList& oldTextures);
