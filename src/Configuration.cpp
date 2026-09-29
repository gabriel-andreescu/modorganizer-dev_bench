#include "Configuration.h"
#include "Tools/Registry.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qtenvironmentvariables.h>
#include <uibase/imoinfo.h>

namespace Bench {
QString ConfigurationPath(const MOBase::IOrganizer* a_organizer) {
    auto portable = QCoreApplication::applicationDirPath() + "/ModOrganizer.ini";
    if (QFileInfo::exists(QCoreApplication::applicationDirPath() + "/portable.txt")) {
        return portable;
    }
    const auto args = QCoreApplication::arguments();
    QString explicitInstance;
    for (int i = 1; i < args.size(); ++i) {
        if ((args[i] == "--instance" || args[i] == "-i") && i + 1 < args.size()) {
            explicitInstance = args[i + 1];
        }
        if (args[i].startsWith("--instance=")) {
            explicitInstance = args[i].mid(11);
        }
    }
    const QDir global(qEnvironmentVariable("LOCALAPPDATA") + "/ModOrganizer");
    if (!explicitInstance.isEmpty()) {
        const auto file = global.filePath(explicitInstance + "/ModOrganizer.ini");
        if (QFileInfo::exists(file)) {
            return file;
        }
    }
    QStringList candidates {portable};
    for (const auto& directory : global.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        candidates.push_back(global.filePath(directory + "/ModOrganizer.ini"));
    }
    QStringList matches;
    for (const auto& file : candidates) {
        if (!QFileInfo::exists(file)) {
            continue;
        }
        const QSettings settings(file, QSettings::IniFormat);
        const auto base = settings.value("Settings/base_directory", QFileInfo(file).absolutePath()).toString();
        if (QDir(base).canonicalPath().compare(QDir(a_organizer->basePath()).canonicalPath(), Qt::CaseInsensitive)
            == 0) {
            matches.push_back(file);
        }
    }
    if (matches.size() != 1) {
        throw ToolError(409, "Cannot uniquely locate active MO2 configuration. Launch MO2 with --instance <name>");
    }
    return matches.front();
}

QString InstanceName(const MOBase::IOrganizer* a_organizer) {
    QDir configuration;
    try {
        configuration = QFileInfo(ConfigurationPath(a_organizer)).absoluteDir();
    } catch (const ToolError& error) {
        qWarning("Dev Bench cannot name this MO2 instance: %s", error.what());
        return {};
    }
    const QDir global(qEnvironmentVariable("LOCALAPPDATA") + "/ModOrganizer");
    const auto parent = QFileInfo(configuration.absolutePath()).absolutePath();
    return QDir(parent).canonicalPath().compare(global.canonicalPath(), Qt::CaseInsensitive) == 0
               ? configuration.dirName()
               : QString();
}
}
