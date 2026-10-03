# Development

The plugin uses
[ModOrganizerPluginKit (MOPK)](https://github.com/gabriel-andreescu/ModOrganizerPluginKit)
for its build, package and release rules. The stdio bridge is TypeScript,
compiled into a standalone executable with Bun. Building it requires Node.js and
npm, which install the pinned Bun.

## Build

```powershell
xmake f --mo2=2.5.2
xmake
xmake package
xmake f --mo2=2.5.3beta12
xmake package
```

Each ZIP contains the build for its MO2 release, for example
`build/dist/dev_bench/dev_bench-0.1.1-MO2-2.5.2.zip`. See MOPK's
[deployment and packaging](https://github.com/gabriel-andreescu/ModOrganizerPluginKit/blob/main/docs/plugin-authors/tooling/packaging.md)
for local deployment.

The native tool descriptors generate the bridge's offline catalog
(`bridge/src/tools-fallback.json`) and the
[tool reference](../automation/tools.md). After changing a descriptor,
regenerate both with `xmake run ToolCatalog`.

To work on the bridge alone, run from `bridge/`:

```sh
npm ci
npm run build
npm test
npm run compile
```

## Validation

```powershell
xmake test
xmake check clang.tidy -f 'src/**.cpp'
```

`xmake test` runs the native tests, the bridge tests and a check that the
offline catalog and tool reference are current. Tests cover registry errors,
event delivery, Qt control interaction and bridge discovery. The pre-commit
hooks format C++, Markdown, YAML, JSON and Lua, and lint the bridge. Live
validation uses a separate portable MO2 installation with disposable mods and
profiles.

The C extension test exercises registration, invocation, events and shutdown
through the exported API. An opt-in direct MCP smoke check is available as
`node --import tsx test/live.ts <mcp-url>` from `bridge/`. The executable editor
workflow check is `node --import tsx test/executables.live.ts <http-url>`. Run
it against a disposable MO2 installation. It creates temporary launch entries,
checks saved configuration and restores the original ordering. Category
definition validation runs as
`node --import tsx test/categories.live.ts <http-url>` and checks native
creation, hierarchy, renaming, ordering, persistence, cancellation and removal
with temporary categories. `node --import tsx test/conflicts.live.ts <http-url>`
uses two temporary mods to check selected-mod inspection, pagination, filter
restoration, provider changes and native hide/unhide.

## Structure

Each MO2 process hosts one loopback endpoint. A tool registry owns JSON schemas
and handlers. MCP and REST adapters expose that registry. MO2 calls execute on
its Qt thread.

- `src/Tools/`: shared descriptors, the offline catalog source, handler
  registration and input validation.
- `src/Transport/`: MCP and REST adapters and live process discovery records.
- `src/Native/`: native dialog, menu and widget interaction.
- Domain modules such as `src/Mods/` and `src/Profiles/`: MO2 interface calls
  and their native dialogs.
- `src/MainThread.*`: UI dispatch, timeout ownership and liveness.
- `bridge/`: standalone MCP stdio companion.
- `include/`: MIT-licensed extension interface, its companion loader and the Qt
  handler adapter for consuming plugins.

The cpp-mcp package patches the MCP server to expose its HTTP server and to
listen on the socket the host binds. The host binds it exclusively, so
concurrent MO2 processes cannot reuse the same listener address. All components
use the same bundled ordered_json header. The MCP methods read the synchronized
registry directly, including tools added by other plugins.
