#include "ProjectRoot.h"

#include <QtGlobal>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

QString resolveDriverTestRoot()
{
    QByteArray e = qgetenv("DRIVER_TEST_ROOT");
    if (!e.isEmpty()) {
        QString p = QString::fromLocal8Bit(e);
        if (QFileInfo::exists(QDir(p).filePath(QStringLiteral("inner/Makefile"))))
            return QDir(p).absolutePath();
    }

    const QString fixed = QStringLiteral("/home/kali/driver_test");
    if (QFileInfo::exists(QDir(fixed).filePath(QStringLiteral("inner/Makefile"))))
        return fixed;

    const QString appPath = QCoreApplication::applicationDirPath();

    {
        QDir d(appPath);
        if (d.dirName() == QStringLiteral("build") && d.cdUp() && d.dirName() == QStringLiteral("outer") && d.cdUp()) {
            if (QFileInfo::exists(d.filePath(QStringLiteral("inner/Makefile"))))
                return d.absolutePath();
        }
    }
    {
        QDir d(appPath);
        if (d.dirName() == QStringLiteral("outer") && d.cdUp()) {
            if (QFileInfo::exists(d.filePath(QStringLiteral("inner/Makefile"))))
                return d.absolutePath();
        }
    }

    QDir cur = QDir::current();
    if (QFileInfo::exists(cur.filePath(QStringLiteral("inner/Makefile"))))
        return cur.absolutePath();

    return cur.absolutePath();
}
