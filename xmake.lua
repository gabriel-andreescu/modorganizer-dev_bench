set_xmakever("3.1.1")
set_project("dev_bench")
set_policy("package.requires_lock", true)

local version = "0.1.0"

add_repositories("mopk https://github.com/gabriel-andreescu/ModOrganizerPluginKit.git")
add_repositories("xmake-luals https://github.com/gabriel-andreescu/xmake-luals.git")
add_addons("mopk 0.1.0", "xmake-luals 0.1.0")
includes("@addon/mopk/project", "@addon/mopk/native", "@addon/xmake-luals/luals")
includes("xmake/cpp-mcp")

-- Dependencies

add_requires("cpp-mcp 2026.07.15", { system = false })

-- Build targets

target("Plugin", function()
    set_default(false)
    set_basename("dev_bench")
    set_version(version)
    add_rules("@addon/mopk/plugin")
    add_files("$(projectdir)/src/**.cpp", "$(projectdir)/src/**.h|PCH.h")
    add_includedirs("$(projectdir)/src", "$(projectdir)/include")
    set_pcxxheader("$(projectdir)/src/PCH.h")
    add_defines("WIN32_LEAN_AND_MEAN", "QT_NO_KEYWORDS")
    add_cxflags("/bigobj", { tools = "cl" })
    add_packages("qt6base", "mo2-uibase", "cpp-mcp")
end)

target("Bridge", function()
    set_kind("phony")
    set_default(false)
    add_tests("unit")
    on_build(function(target)
        import("core.project.depend")
        local bridge = path.join(os.projectdir(), "bridge")
        local executable = path.join(bridge, "dist", "dev-bench-bridge.exe")
        local inputs = os.files(path.join(bridge, "src", "**"))
        table.join2(inputs, path.join(bridge, "package.json"), path.join(bridge, "package-lock.json"))
        table.join2(inputs, path.join(bridge, "tsconfig.json"))
        depend.on_changed(function()
            local npm = is_host("windows") and "npm.cmd" or "npm"
            local options = { curdir = bridge }
            os.vrunv(npm, { "ci", "--no-audit", "--no-fund" }, options)
            os.vrunv(npm, { "run", "build" }, options)
            os.vrunv(npm, { "run", "compile" }, options)
        end, {
            dependfile = target:dependfile(executable),
            files = inputs,
            changed = not os.isfile(executable),
        })
    end)
    on_test(function()
        local npm = is_host("windows") and "npm.cmd" or "npm"
        return os.execv(npm, { "test" }, { curdir = path.join(os.projectdir(), "bridge"), try = true }) == 0
    end)
    add_installfiles("bridge/dist/dev-bench-bridge.exe", { prefixdir = "dev_bench" })
    add_installfiles(
        "bridge/dist/THIRD-PARTY-NOTICES.txt",
        { prefixdir = "dev_bench/licenses", filename = "dev-bench-bridge.txt" }
    )
end)

-- Generates the bridge's offline catalog and the tool reference from the native descriptors.
target("ToolCatalog", function()
    set_kind("binary")
    set_default(false)
    set_languages("c++23")
    add_rules("@addon/mopk/native.compiler", "qt.console")
    add_frameworks("QtCore")
    add_files("tools/catalog.cpp", "src/Tools/Catalog.cpp", "src/Settings/Sections.cpp")
    add_includedirs("src")
    add_deps("Bridge")
    add_packages("qt6base", "cpp-mcp")
    add_tests("current")
    on_run(function(target)
        os.execv(target:targetfile(), { path.join(os.projectdir(), "bridge", "src", "tools-fallback.json") })
        os.execv("node", { path.join(os.projectdir(), "scripts", "generate-reference.mjs") })
    end)
    on_test(function(target)
        local directory = os.tmpfile() .. ".dir"
        os.mkdir(directory)
        local catalog = path.join(directory, "tools-fallback.json")
        local reference = path.join(directory, "tools.md")
        os.execv(target:targetfile(), { catalog })
        os.execv("node", { path.join(os.projectdir(), "scripts", "generate-reference.mjs"), catalog, reference })
        local current = io.readfile(catalog)
                == io.readfile(path.join(os.projectdir(), "bridge", "src", "tools-fallback.json"))
            and io.readfile(reference) == io.readfile(path.join(os.projectdir(), "docs", "automation", "tools.md"))
        os.tryrm(directory)
        if not current then
            print("The tool catalog is stale. Run `xmake run ToolCatalog`.")
        end
        return current
    end)
end)

-- Tests

local function unit_test(name, frameworks, files, options)
    options = options or {}
    target(name, function()
        set_kind("binary")
        set_default(false)
        set_languages("c++23")
        add_rules("@addon/mopk/native.compiler", "qt.console")
        add_frameworks(frameworks)
        add_files(files)
        add_includedirs("src", "include")
        add_defines(options.defines or {})
        add_packages("qt6base", "cpp-mcp")
        add_tests("default", { runargs = options.runargs })
    end)
end

local major, minor, patch = version:match("^(%d+)%.(%d+)%.(%d+)$")

unit_test("RegistryTests", { "QtCore" }, {
    "tests/registry.cpp",
    "src/Tools/Registry.cpp",
    "src/Tools/JsonSchema.cpp",
    "src/EventBus.cpp",
})
unit_test("DialogTests", { "QtCore", "QtGui", "QtWidgets" }, {
    "tests/dialogs.cpp",
    "src/Dialogs/Actions.cpp",
    "src/Dialogs/Choices.cpp",
    "src/Dialogs/Fields.cpp",
    "src/Dialogs/FileDialogActions.cpp",
    "src/Native/WidgetValue.cpp",
    "src/Tools/Registry.cpp",
    "src/Tools/JsonSchema.cpp",
}, { runargs = { "-platform", "offscreen" } })
unit_test(
    "NativeOperationTests",
    { "QtCore", "QtGui", "QtWidgets" },
    { "tests/native_operation.cpp", "src/Native/Operation.cpp", "src/Native/DialogSnapshot.cpp", "src/EventBus.cpp" },
    { runargs = { "-platform", "offscreen" } }
)
unit_test("ExtensionTests", { "QtCore" }, {
    "tests/extensions.cpp",
    "src/ExtensionApi.cpp",
    "src/Tools/Registry.cpp",
    "src/Tools/Extensions.cpp",
    "src/Tools/JsonSchema.cpp",
    "src/EventBus.cpp",
}, {
    defines = {
        "MOPK_VERSION_MAJOR=" .. major,
        "MOPK_VERSION_MINOR=" .. minor,
        "MOPK_VERSION_PATCH=" .. patch,
    },
})
unit_test("QtAdapterTests", { "QtCore" }, { "tests/qt_adapter.cpp", "include/DevBenchAPI.cpp" })
unit_test("LogTailTests", { "QtCore" }, { "tests/log_tail.cpp", "src/Logs/Tail.cpp" })
unit_test("SsimTests", { "QtCore", "QtGui" }, { "tests/ssim.cpp", "src/Capture/Ssim.cpp" })
unit_test("CaptureTests", { "QtCore", "QtGui" }, {
    "tests/capture.cpp",
    "src/Capture/Artifacts.cpp",
    "src/Capture/Completion.cpp",
    "src/Capture/Providers.cpp",
    "src/Capture/Ssim.cpp",
    "src/Tools/Extensions.cpp",
    "src/EventBus.cpp",
})

-- Packages

target("dev_bench", function()
    set_version(version)
    add_rules("@addon/mopk/package", { targets = { "Plugin", "Bridge" } })
    add_installfiles("COPYING", "EXCEPTIONS", { prefixdir = "dev_bench" })
    add_installfiles("docs/(**)|maintainers/**", { prefixdir = "dev_bench/docs" })
    add_installfiles("include/(DevBenchAPI.*)", "include/(DevBenchQt.h)", { prefixdir = "dev_bench/include" })
    add_installfiles("licenses/(**)", { prefixdir = "dev_bench/licenses" })
end)
