#include "Host/Host.h"
#include "Json.h"
#include <qpointer.h>
#include <uibase/idownloadmanager.h>
#include <uibase/ipluginlist.h>

namespace Bench {
void Host::ConnectModEvents() {
    const QPointer<Host> self(this);
    _organizer->modList()->onModInstalled([self](auto* a_mod) {
        if (self) {
            self->_events.Publish("mods.installed", {{"name", Utf8(a_mod->name())}});
        }
    });
    _organizer->modList()->onModRemoved([self](const auto& a_name) {
        if (self) {
            self->_events.Publish("mods.removed", {{"name", Utf8(a_name)}});
        }
    });
    _organizer->modList()->onModMoved([self](const auto& a_name, int a_oldPriority, int a_priority) {
        if (self) {
            self->_events.Publish(
                "mods.moved",
                {{"name", Utf8(a_name)}, {"oldPriority", a_oldPriority}, {"priority", a_priority}}
            );
        }
    });
    _organizer->modList()->onModStateChanged([self](const auto& a_states) {
        if (self) {
            Json data = Json::object();
            for (const auto& [name, state] : a_states) {
                data[Utf8(name)] = static_cast<int>(state);
            }
            self->_events.Publish("mods.state", data);
        }
    });
}

void Host::ConnectPluginEvents() {
    const QPointer<Host> self(this);
    _organizer->pluginList()->onRefreshed([self] {
        if (self) {
            self->_events.Publish("plugins.refreshed", Json::object());
        }
    });
    _organizer->pluginList()->onPluginMoved([self](const auto& a_name, int a_oldPriority, int a_priority) {
        if (self) {
            self->_events.Publish(
                "plugins.moved",
                {{"name", Utf8(a_name)}, {"oldPriority", a_oldPriority}, {"priority", a_priority}}
            );
        }
    });
    _organizer->pluginList()->onPluginStateChanged([self](const auto& a_states) {
        if (self) {
            Json data = Json::object();
            for (const auto& [name, state] : a_states) {
                data[Utf8(name)] = static_cast<int>(state);
            }
            self->_events.Publish("plugins.state", data);
        }
    });
}

void Host::ConnectLifecycleEvents() {
    const QPointer<Host> self(this);
    _organizer->onProfileChanged([self](const auto&, const auto& a_profile) {
        if (self) {
            self->_events.Publish("profiles.changed", {{"name", a_profile ? Utf8(a_profile->name()) : ""}});
        }
    });
    _organizer->onFinishedRun([self](const auto& a_binary, unsigned int a_code) {
        if (self) {
            self->_events.Publish("executables.finished", {{"binary", Utf8(a_binary)}, {"exitCode", a_code}});
        }
    });
    _organizer->downloadManager()->onDownloadComplete([self](int a_id) {
        if (self) {
            self->_events.Publish("downloads.complete", {{"id", a_id}});
        }
    });
    _organizer->downloadManager()->onDownloadFailed([self](int a_id) {
        if (self) {
            self->_events.Publish("downloads.failed", {{"id", a_id}});
        }
    });
}
}
