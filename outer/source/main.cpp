#include "MainWindow.h"
#include "engine_power.h"
#include "EditorPrefs.h"
#include "ProjectRoot.h"
#include "textures_path.h"

#include <QApplication>
#include <QDir>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("DriverTest"));
    QApplication::setApplicationName(QStringLiteral("SceneEditor"));

    const QString root = resolveDriverTestRoot();
    setInnerDirectoryOverride(QDir(root).filePath(QStringLiteral("inner")).toStdString());

    EditorPrefs::instance().load();
    applyEditorTheme(EditorPrefs::instance().theme());

    int power = 2;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QLatin1String("--power") && i + 1 < args.size())
            power = args[++i].toInt();
    }
    engine::setPowerLevel(power);

    MainWindow w;
    w.show();
    return app.exec();
}
