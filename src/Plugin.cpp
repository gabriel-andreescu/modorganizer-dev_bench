#include "Plugin.h"
#include "Host/Host.h"
#include <QDebug>
#include <exception>
#include <memory>
#include <qlist.h>
#include <qlogging.h>
#include <qmainwindow.h>
#include <uibase/imoinfo.h>
#include <uibase/pluginsetting.h>
#include <uibase/versioninfo.h>
DevBenchPlugin::DevBenchPlugin() = default;
DevBenchPlugin::~DevBenchPlugin() = default;

bool DevBenchPlugin::init(MOBase::IOrganizer* a_organizer) {
    a_organizer->onUserInterfaceInitialized([this, a_organizer](QMainWindow*) {
        try {
            _host = std::make_unique<Bench::Host>(a_organizer, this);
            _host->Start();
        } catch (const std::exception& error) {
            qCritical() << "Dev Bench startup failed:" << error.what();
            _host.reset();
        }
    });
    return true;
}

QString DevBenchPlugin::name() const {
    return "Dev Bench";
}

QString DevBenchPlugin::author() const {
    return "GabonZ";
}

QString DevBenchPlugin::description() const {
    return "Local MCP and HTTP automation for Mod Organizer 2.";
}

MOBase::VersionInfo DevBenchPlugin::version() const {
    return {MOPK_VERSION_MAJOR, MOPK_VERSION_MINOR, MOPK_VERSION_PATCH, MOBase::VersionInfo::RELEASE_FINAL};
}

QList<MOBase::PluginSetting> DevBenchPlugin::settings() const {
    return {{"port", "Preferred loopback MCP/HTTP port.", 8930}};
}
