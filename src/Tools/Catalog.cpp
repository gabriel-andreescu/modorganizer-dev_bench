#include "Tools/Catalog.h"
#include "Json.h"
#include "Mods/ConflictFlags.h"
#include "Settings/Sections.h"

namespace Bench {
namespace {
    Json Field(const char* a_type) {
        return {{"type", a_type}};
    }

    Json Described(Json a_schema, const char* a_description) {
        a_schema["description"] = a_description;
        return a_schema;
    }

    Json Tool(
        const char* a_name,
        const char* a_description,
        Json a_actions,
        Json a_properties = Json::object(),
        bool a_readOnly = false
    ) {
        a_properties["action"] = {{"type", "string"}, {"enum", a_actions}};
        return {
            {"name", a_name},
            {"description", a_description},
            {"readOnly", a_readOnly},
            {"inputSchema", {{"type", "object"}, {"properties", a_properties}, {"additionalProperties", false}}},
        };
    }

    Json InspectTool() {

        return Tool(
            "inspect",
            "Read MO2 process identity, health and connection setup. Health is answered without waiting for the UI.",
            {"health", "bridge"}
        );
    }

    Json ModFilterSchema() {
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        Json criterion = {{"type", "object"}, {"required", {"index", "state"}}, {"additionalProperties", false}};
        criterion["properties"]["index"] = number;
        criterion["properties"]["state"] = {{"type", "string"}, {"enum", {"inactive", "include", "exclude"}}};

        Json properties = {{"text", Field("string")}, {"grouping", number}, {"separators", number}};
        properties["mode"] = {{"type", "string"}, {"enum", {"all", "any"}}};
        properties["criteria"] = {{"type", "array"}, {"items", criterion}};
        return {{"type", "object"}, {"additionalProperties", false}, {"properties", properties}};
    }

    Json ModValuesSchema() {
        const Json text = Field("string");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        const Json category = {
            {"type", "object"},
            {"required", {"id", "assigned"}},
            {"additionalProperties", false},
            {"properties", {{"id", number}, {"assigned", Field("boolean")}}},
        };
        return {
            {"type", "object"},
            {"additionalProperties", false},
            {
                "properties",
                {
                    {"version", text},
                    {"comments", text},
                    {"notes", Described(text, "editorUpdate: HTML notes")},
                    {"primaryCategory", Described(number, "editorUpdate: an assigned category ID")},
                    {
                        "categories",
                        Described({{"type", "array"}, {"items", category}}, "editorUpdate: ordered changes"),
                    },
                    {"newestVersion", text},
                    {"nexusId", number},
                    {"game", text},
                    {"url", text},
                    {"installationFile", text},
                },
            },
        };
    }

    Json ModsTool() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        Json conflictFlags = Json::array();
        for (const auto& [name, label] : kModConflictFlags) {
            conflictFlags.push_back(name);
        }
        return Tool(
            "mods",
            "List and inspect mods, metadata, native conflict flags and separators, and change enablement, ordering, categories or metadata. list/get include conflicts.redundant, flags and native tooltip. Optional conflict filters list by one flag. Flags mirror MO2 cached classification: after file hide/unhide, close the editor and use settings refresh before checking flags. remove schedules native confirmation: answer through dialogs and poll operationStatus for the removed result. manage opens native mod information. editorState discovers category IDs, editorUpdate writes notes/comments/category assignments/primary category immediately, and closeEditor finishes and saves. setIgnoreUpdate invokes the currently available native update action. These menu operations require the mod to pass current list filters. filters discovers native text/criteria/mode/grouping/separator settings, setFilters takes filter and clearFilters invokes native text/criteria reset. Poll operationStatus for native operation outcomes.",
            {
                "list",
                "get",
                "separators",
                "filters",
                "setFilters",
                "clearFilters",
                "create",
                "createSeparator",
                "setEnabled",
                "setPriority",
                "move",
                "rename",
                "remove",
                "metadata",
                "manage",
                "editorState",
                "editorUpdate",
                "closeEditor",
                "setIgnoreUpdate",
                "operationStatus",
                "addCategory",
                "removeCategory",
            },
            {
                {"name", Described(text, "mod or separator name. create/createSeparator: the new name")},
                {"enabled", Described(boolean, "setEnabled: enable the mod. setIgnoreUpdate: ignore its update")},
                {"priority", Described(number, "setPriority: zero-based position in the mod list")},
                {"separator", Described(text, "move: place the mod immediately after this separator")},
                {"newName", Described(text, "rename: the new name")},
                {"category", Described(text, "addCategory/removeCategory: category name")},
                {
                    "conflict",
                    {
                        {"type", "string"},
                        {"enum", conflictFlags},
                        {"description", "list: only mods with this native conflict flag, including redundant"},
                    },
                },
                {"filter", Described(ModFilterSchema(), "setFilters: text, mode, grouping, separators and criteria")},
                {"values", Described(ModValuesSchema(), "metadata/editorUpdate: fields to write")},
            }
        );
    }

    Json CategoriesTool() {
        const Json text = Field("string");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "categories",
            "Manage native category definitions, hierarchy, order and Nexus mappings. list opens a snapshot and closes without applying. Use editorState for open edits. Other actions stage edits until accept, and cancel discards them. All operations are scheduled: poll operationStatus. Targets use row from editorState because category IDs can repeat, and edits return fresh row positions. setOrder takes all rows and setMappings takes Nexus ids. Changing/removing IDs follows MO2's native category behavior, including existing mod assignments.",
            {
                "list",
                "manage",
                "editorState",
                "create",
                "update",
                "remove",
                "setOrder",
                "setMappings",
                "accept",
                "cancel",
                "operationStatus",
            },
            {
                {"row", Described(number, "update/remove/setMappings: row position from the latest editor response")},
                {
                    "rows",
                    Described(
                        {{"type", "array"}, {"items", number}, {"uniqueItems", true}},
                        "setOrder: every current row position once, in the requested order"
                    ),
                },
                {
                    "ids",
                    Described(
                        {{"type", "array"}, {"items", number}, {"uniqueItems", true}},
                        "setMappings: Nexus category IDs from nexusCategories"
                    ),
                },
                {
                    "values",
                    Described(
                        {
                            {"type", "object"},
                            {"additionalProperties", false},
                            {"properties", {{"id", number}, {"name", text}, {"parentId", number}}},
                        },
                        "create/update: id, name and parentId (0 for a root category)"
                    ),
                },
            }
        );
    }

    Json PluginsTool() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        const Json strings = {{"type", "array"}, {"items", text}};
        return Tool(
            "plugins",
            "Read the entire plugin order including disabled plugins, masters, origin and flags, and enable plugins or change ordering. forced plugins cannot be enabled or disabled. Changes return after MO2 saves the profile's plugin list.",
            {"list", "setEnabled", "setPriority", "setLoadOrder"},
            {
                {"name", Described(text, "setEnabled/setPriority: plugin filename")},
                {"enabled", Described(boolean, "setEnabled: enable the plugin")},
                {"priority", Described(number, "setPriority: zero-based position in the full plugin list")},
                {"names", Described(strings, "setLoadOrder: every current plugin exactly once, in the new order")},
            }
        );
    }

    Json ExecutableValuesSchema() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        return {
            {"type", "object"},
            {"additionalProperties", false},
            {
                "properties",
                {
                    {"title", text},
                    {"binary", text},
                    {"workingDirectory", text},
                    {"arguments", text},
                    {"steamAppID", text},
                    {"overwriteSteamAppID", boolean},
                    {"createFilesInMod", boolean},
                    {"outputMod", text},
                    {"forceLoadLibraries", boolean},
                    {"useApplicationIcon", boolean},
                    {"minimizeToSystemTray", Described(boolean, "MO2 2.5.3beta12 only")},
                    {"hide", boolean},
                },
            },
        };
    }

    Json LibrariesSchema() {
        const Json text = Field("string");
        const Json library = {
            {"type", "object"},
            {"additionalProperties", false},
            {"required", {"process", "library", "enabled"}},
            {"properties", {{"process", text}, {"library", text}, {"enabled", Field("boolean")}}},
        };
        return {{"type", "array"}, {"items", library}};
    }

    Json ExecutablesTool() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        const Json strings = {{"type", "array"}, {"items", text}};
        return Tool(
            "executables",
            "Read/launch executables and manage every native editor field/order. list includes an INI snapshot that may lag live changes. snapshot reads current live configuration with the editor closed. Native operations are scheduled: poll operationStatus and inspect dialogs. Apply/accept commits live edits and cancel discards unapplied changes. libraries/setLibraries validate live titles with the editor closed.",
            {
                "list",
                "launch",
                "status",
                "manage",
                "editorState",
                "snapshot",
                "create",
                "clone",
                "addFromFile",
                "update",
                "remove",
                "setOrder",
                "reset",
                "apply",
                "accept",
                "cancel",
                "configureLibraries",
                "operationStatus",
                "libraries",
                "setLibraries",
            },
            {
                {"name", Described(text, "configured title. launch: title or executable path")},
                {"args", Described(strings, "launch: command-line arguments")},
                {"cwd", Described(text, "launch: working directory")},
                {"profile", Described(text, "launch: profile to run with (default the active profile)")},
                {"overwrite", Described(text, "launch: mod that receives new files instead of Overwrite")},
                {"names", Described(strings, "setOrder: every editor title exactly once, in the new order")},
                {"enabled", Described(boolean, "setLibraries: overall force-load state")},
                {"values", Described(ExecutableValuesSchema(), "update: editor fields to change")},
                {
                    "libraries",
                    Described(LibrariesSchema(), "setLibraries: replacement {process, library, enabled} rows"),
                },
            }
        );
    }

    Json DownloadsTool() {
        const Json text = Field("string");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        const Json strings = {{"type", "array"}, {"items", text}};
        return Tool(
            "downloads",
            "List downloaded archives with metadata, start URL/Nexus downloads or resolve download IDs.",
            {"list", "start", "nexus", "path"},
            {
                {"urls", Described(strings, "start: URLs to download")},
                {"modId", Described(number, "nexus: Nexus mod ID")},
                {"fileId", Described(number, "nexus: Nexus file ID")},
                {"id", Described(number, "path: download manager ID from list")},
            }
        );
    }

    Json NexusTool() {
        const Json text = Field("string");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "nexus",
            "Retrieve full Nexus mod details including description through MO2's authenticated Nexus integration, or file details/download URLs. Network actions return requestId. Poll status for the complete response or failure. cached reads installed meta.ini, live mod metadata and previous responses.",
            {"description", "fileInfo", "downloadUrls", "status", "cached"},
            {
                {"name", Described(text, "installed mod whose Nexus IDs to use. cached: required")},
                {"game", Described(text, "Nexus game name when not using name (default the managed game)")},
                {"modId", Described(number, "Nexus mod ID when not using name")},
                {"fileId", Described(number, "fileInfo/downloadUrls: Nexus file ID")},
                {"requestId", Described(text, "status: request to poll. Omit to list this session's requests")},
            },
            true
        );
    }

    Json InstallTool() {
        const Json text = Field("string");
        return Tool(
            "install",
            "Start MO2's native archive installer from an absolute path or downloads filename. Returns a session. Poll status and use dialogs to read/answer FOMOD pages, then verify completed status. Optional separator determines final placement.",
            {"start", "status"},
            {
                {"path", Described(text, "start: absolute archive path")},
                {"download", Described(text, "start: archive filename from downloads list")},
                {"name", Described(text, "start: suggested mod name")},
                {"separator", Described(text, "start: place the installed mod after this separator")},
            }
        );
    }

    Json DialogsTool() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "dialogs",
            "Inspect native dialogs including FOMOD pages, descriptions, images and choices. Use returned IDs to click, toggle, edit, set values, select or cancel, and re-describe after changes. File dialogs expose fileSelection. selectFile takes the dialog id and path, then accept schedules native validation and completion.",
            {
                "describe",
                "choices",
                "click",
                "setChecked",
                "setText",
                "setValue",
                "select",
                "selectFile",
                "accept",
                "cancel",
                "reveal",
                "scroll",
            },
            {
                {"id", Described(number, "control ID from describe. selectFile/accept/cancel: dialog ID")},
                {"checked", Described(boolean, "setChecked: checked state")},
                {"text", Described(text, "setText: text to enter")},
                {"path", Described(text, "selectFile: file to select")},
                {"index", Described(number, "select: zero-based item index")},
                {
                    "value",
                    Described(
                        Json::object(),
                        "setValue: boolean, number, choice text or text matching the control. scroll: absolute position"
                    ),
                },
                {"filter", Described(text, "choices: match group names, option text or descriptions")},
            }
        );
    }

    Json UiTool() {
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "ui",
            "Discover native MO2 window actions and trigger an enabled action by its returned ID. Inspect dialogs after triggering actions that open a window.",
            {"list", "trigger"},
            {{"id", Described(number, "trigger: action ID from list")}}
        );
    }

    Json ProfilesTool() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        return Tool(
            "profiles",
            "Read/select profiles or open native create/copy/rename/remove/settings/save-transfer workflows. Mutations are scheduled, so inspect operationStatus and dialogs for name entry, confirmation, character selection and completion. name selects the source profile, and enabled controls local save/INI settings.",
            {
                "list",
                "select",
                "manage",
                "create",
                "copy",
                "rename",
                "remove",
                "setLocalSaves",
                "setLocalSettings",
                "transferSaves",
                "operationStatus",
            },
            {
                {"name", Described(text, "profile to select, copy, rename, remove, configure or transfer saves for")},
                {"enabled", Described(boolean, "setLocalSaves/setLocalSettings: use profile-local saves or INIs")},
            }
        );
    }

    Json ConflictsTool() {
        const Json text = Field("string");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        const Json options = {
            {"type", "object"},
            {"additionalProperties", false},
            {"properties", {{"includeUnique", Field("boolean")}, {"allProviders", Field("boolean")}}},
        };
        return Tool(
            "conflicts",
            "Inspect per-mod file conflicts, providers and physical files, and hide or unhide files through MO2's native editors. inspect takes mods (exact names) and/or modFilter (name substring), and omitting both inspects the mod list. It schedules native snapshots: poll operationStatus. Snapshots exclude unique files by default, temporarily clear mod-list filters and restore them after closing their editors. list inspects the currently open native editor after mods manage. directory lists physical mod files including .mohidden. hideFiles/unhide use the native file tree on paths, including directories. archive identifies entries supplied by an archive, and provider text is MO2's native display. inspect also returns a structured providers array, winner first. files info/resolve queries providers directly. hide and preview select paths from this view and invoke native actions, so answer dialogs and poll operationStatus. Archive parsing in settings workarounds is required for archive members to appear.",
            {"inspect", "list", "directory", "hide", "hideFiles", "unhide", "preview", "operationStatus"},
            {
                {"mods", Described({{"type", "array"}, {"items", text}}, "inspect: exact mod names")},
                {"modFilter", Described(text, "inspect: mod name substring")},
                {"modOffset", Described(number, "inspect: first mod to return (default 0)")},
                {"modLimit", Described(number, "inspect: mods to return (default 10)")},
                {"path", Described(text, "directory: relative folder inside the mod")},
                {
                    "view",
                    Described(
                        {{"type", "string"}, {"enum", {"advanced", "winning", "losing", "unique"}}},
                        "inspect/list: native conflict view (default advanced)"
                    ),
                },
                {"filter", Described(text, "inspect/list: native file filter text")},
                {"offset", Described(number, "inspect/list: first file per mod (default 0)")},
                {"limit", Described(number, "inspect/list: files per mod (default 100)")},
                {
                    "paths",
                    Described(
                        {{"type", "array"}, {"minItems", 1}, {"items", text}},
                        "hide/preview: paths from list. hideFiles/unhide: relative paths from directory"
                    ),
                },
                {"options", Described(options, "inspect/list: advanced view unique-file and provider display")},
            }
        );
    }

    Json FilesTool() {
        const Json text = Field("string");
        const Json strings = {{"type", "array"}, {"items", text}};
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "files",
            "Find virtual files, list virtual directories, or resolve a file. info returns one directory of virtual files with virtual path, MO2 resolvedPath, winner archive and structured origins in native order (winner first). Optional filters match filenames, and offset/limit page info (default 100). Archive members require MO2 archive parsing. Their resolvedPath can be a virtual location inside the mod, not a loose file.",
            {"find", "directories", "resolve", "info"},
            {
                {"path", Described(text, "virtual path. resolve: a file. Other actions: a directory")},
                {"filters", Described(strings, "find/info: filename globs")},
                {"offset", Described(number, "info: first file (default 0)")},
                {"limit", Described(number, "info: files to return (default 100)")},
            }
        );
    }

    Json SettingsTool() {
        const Json text = Field("string");
        Json sections = Json::array();
        for (const auto& section : SettingsSections()) {
            sections.push_back(section.at("id"));
        }
        return Tool(
            "settings",
            "Root list discovers MO2 settings sections. Native section get/set opens the corresponding editor, and set values maps discovered control objectNames to values. Poll operationStatus, then apply or cancel. Native confirmations, restart and refresh remain observable. Section plugins list discovers plugins/keys, get/set uses plugin and key with value for immediate live edits. manage can open any native section, including the Plugins tab.",
            {"list", "get", "set", "refresh", "manage", "apply", "cancel", "operationStatus"},
            {
                {"plugin", Described(text, "section plugins: plugin name")},
                {"key", Described(text, "section plugins get/set: setting key")},
                {"value", Described(Json::object(), "section plugins set: new JSON value")},
                {
                    "section",
                    Described(
                        {{"type", "string"}, {"enum", sections}},
                        "settings section. Omit with list to discover them"
                    ),
                },
                {"values", Described({{"type", "object"}}, "native section set: control objectName to value")},
            }
        );
    }

    Json EventsTool() {
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "events",
            "Read semantic MO2 events after a sequence number, optionally waiting for new events.",
            {"poll", "wait"},
            {
                {"since", Described(number, "return events after this sequence number (default 0)")},
                {"timeoutMs", Described(number, "wait: how long to wait for a new event")},
            },
            true
        );
    }

    Json LogsTool() {
        return Tool(
            "logs",
            "List log sources/files without their contents, or read one source's last lines. read defaults to this process's live log and 10 lines. lines accepts any nonnegative count with no configured maximum. The file source requires a filename from list.",
            {"list", "read"},
            {
                {
                    "source",
                    Described({{"type", "string"}, {"enum", {"live", "file"}}}, "read: log source (default live)"),
                },
                {"file", Described(Field("string"), "read with source file: filename from list")},
                {
                    "lines",
                    Described({{"type", "integer"}, {"minimum", 0}, {"default", 10}}, "read: last lines to return"),
                },
            },
            true
        );
    }

    Json ScenarioTool() {
        const Json text = Field("string");
        const Json boolean = Field("boolean");
        const Json number = {{"type", "integer"}, {"minimum", 0}};
        return Tool(
            "scenario",
            "Run registered tools sequentially, or wait for an event topic. Returns per-step data and errors. An installation step returns its session. Interact with dialogs in subsequent calls.",
            {"run"},
            {
                {
                    "steps",
                    Described(
                        {
                            {"type", "array"},
                            {
                                "items",
                                {
                                    {"type", "object"},
                                    {
                                        "properties",
                                        {
                                            {"tool", text},
                                            {"args", {{"type", "object"}}},
                                            {"waitFor", text},
                                            {"timeoutMs", number},
                                        },
                                    },
                                    {"additionalProperties", false},
                                },
                            },
                        },
                        "run: {tool, args} calls or {waitFor, timeoutMs} event waits"
                    ),
                },
                {"continueOnError", Described(boolean, "run: continue after a failed step (default false)")},
            }
        );
    }
}

Json CaptureTool(const std::vector<std::string>& a_providers) {
    Json kinds = Json::array({"auto", "native", "windows", "providers", "extensions"});
    for (const auto& provider : a_providers) {
        kinds.push_back(provider);
    }
    const Json text = Field("string");
    const Json integer = {{"type", "integer"}, {"minimum", 0}};
    const Json boolean = Field("boolean");
    auto kind = Described(
        text,
        "auto (default) | native | windows | providers | extensions | a registered provider key"
    );
    kind["enum"] = kinds;
    const Json properties = {
        {"kind", kind},
        {"checkpointId", Described(text, "stable file stem and correlation key, required for captures")},
        {"recording", Described(text, "recording name, for correlation (default 'adhoc')")},
        {"variant", Described(text, "variant under test, for correlation (default 'default')")},
        {"window", Described(integer, "native: window id from kind=windows (default the main window)")},
        {"allowNative", Described(boolean, "auto: fall back to native when no provider is registered (default false)")},
        {"excludeUi", Described(boolean, "request a capture without UI. native cannot honor it (default true)")},
        {"outDir", Described(text, "override the capture directory")},
        {"timeoutMs", Described(integer, "provider: how long to wait for the image (default 8000)")},
        {"pollMs", Described(integer, "provider: poll interval while waiting (default 100)")},
        {"runId", Described(text, "correlation ID copied to the result and request ID")},
        {"repeat", Described(integer, "repeat index, appended to the file stem as `__r<repeat>`")},
        {
            "sceneMismatch",
            Described(boolean, "mark the capture inconclusive because the scene differs (default false)"),
        },
        {"subrect", Described(Field("object"), "provider: capture rectangle {x,y,w,h} in 0..1 UV")},
        {
            "golden",
            Described(
                text,
                "path to a reference image to SSIM-compare against (absolute, or relative to the MO2 install). Adds "
                "{ssim,threshold,passed} to the result"
            ),
        },
        {"threshold", Described(Field("number"), "golden: SSIM >= threshold passes (default 0.98)")},
        {
            "regions",
            Described(
                Field("array"),
                "golden: [{name,x,y,w,h,threshold?}] in 0..1 UV, scored independently. Passes only if every region "
                "passes"
            ),
        },
    };
    return {
        {"name", "capture"},
        {
            "description",
            "Capture an image and return its file path, correlation metadata and the image. kind='auto' uses the sole "
            "registered capture provider. With none it returns 404 unless allowNative is true, and with several it "
            "returns 400. kind='native' captures an MO2 window (the main window unless window is given) and always "
            "includes UI. kind='windows' lists this process's visible windows. kind='providers' lists provider keys and "
            "kind='extensions' their descriptors. Captures other than listings require checkpointId. A successful "
            "golden comparison adds {ssim,threshold,passed,regions?}, and a missing, unreadable or incompatible golden "
            "adds {goldenError}. A scene mismatch or degraded capture makes the comparison inconclusive.",
        },
        {"readOnly", false},
        {"inputSchema", {{"type", "object"}, {"properties", properties}}},
    };
}

Json Catalog() {
    return Json::array({
        InspectTool(),
        ModsTool(),
        CategoriesTool(),
        ConflictsTool(),
        PluginsTool(),
        ExecutablesTool(),
        DownloadsTool(),
        NexusTool(),
        InstallTool(),
        DialogsTool(),
        CaptureTool({}),
        UiTool(),
        ProfilesTool(),
        FilesTool(),
        SettingsTool(),
        LogsTool(),
        EventsTool(),
        ScenarioTool(),
    });
}
}
