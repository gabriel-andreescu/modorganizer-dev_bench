#pragma once
#include "Capture/Tool.h"
#include "Categories/Manager.h"
#include "Conflicts/Actions.h"
#include "Dialogs/Actions.h"
#include "EventBus.h"
#include "Executables/Manager.h"
#include "Executables/ProcessTracker.h"
#include "Installation.h"
#include "MainThread.h"
#include "Mods/Actions.h"
#include "NexusActions.h"
#include "Profiles/Manager.h"
#include "Settings/Manager.h"
#include "Tools/Registry.h"
#include "Transport/Runtime.h"
#include "Transport/Server.h"
#include "UiActions.h"
#include <map>
#include <string>

namespace Bench {
class Host : public QObject {
public:
    Host(MOBase::IOrganizer* a_organizer, QObject* a_parent);
    Host(const Host&) = delete;
    Host(Host&&) = delete;
    Host& operator=(const Host&) = delete;
    Host& operator=(Host&&) = delete;
    ~Host() override;
    void Start();

private:
    void RegisterTools();
    [[nodiscard]] ToolRegistry::Handler OnMainThread(ToolRegistry::Handler a_handler);
    [[nodiscard]] std::map<std::string, ToolRegistry::Handler> MainThreadHandlers();
    [[nodiscard]] ToolRegistry::Handler DirectHandler(const std::string& a_name);
    void RefreshCaptureTool();
    void ConnectModEvents();
    void ConnectPluginEvents();
    void ConnectLifecycleEvents();
    MOBase::IOrganizer* _organizer;
    MainThread _main;
    ToolRegistry _registry;
    EventBus _events;
    ProcessTracker _processes;
    Runtime _runtime;
    ModActions _mods;
    NexusActions _nexus;
    ProfileManager _profileManager;
    ExecutableManager _executableManager;
    DialogActions _dialogs;
    SettingsManager _settingsManager;
    CategoryManager _categoryManager;
    ConflictActions _conflicts;
    UiActions _ui;
    Installation _installation;
    Capture::Tool _capture;
    Server _server;
};
}
