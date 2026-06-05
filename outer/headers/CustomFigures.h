#pragma once

#include "SceneFile.h"

#include <QString>
#include <QVector>

struct CustomFigurePreset {
    QString name;
    SceneObject object;
};

QString defaultCustomFiguresCatalogPath(const QString& repoRoot);

bool loadCustomFiguresCatalog(const QString& path, QVector<CustomFigurePreset>& out, QString* errorMsg = nullptr);
bool saveCustomFiguresCatalog(const QString& path, const QVector<CustomFigurePreset>& presets, QString* errorMsg = nullptr);
