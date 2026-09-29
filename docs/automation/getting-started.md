# Automating MO2

Extract the ZIP matching your MO2 version into that installation's `plugins/`
directory. Restart MO2. The plugin setting `Dev Bench / port` controls the
preferred loopback port.

## MCP bridge

The MCP client starts the bridge over stdio. It discovers MO2 on every request
and forwards calls to the running process's REST API. Closing or restarting MO2
leaves the MCP session connected. Offline discovery returns the compiled core
tool catalog, and live calls return an explicit disconnected error. Tools
registered by other plugins appear while MO2 is running.

Configure your MCP client:

```json
{
  "mcpServers": {
    "mo2-dev-bench": {
      "command": "C:/Tools/MO2/plugins/dev_bench/dev-bench-bridge.exe"
    }
  }
}
```

Run `dev-bench-bridge setup` to print configuration. The running plugin also
exposes it through `inspect action=bridge`, `GET /api/tools` and
`plugins/dev_bench/mcp-bridge.json`.

Start MO2 before running automation. Without flags the bridge targets the only
live process. When several are live, select one with
`--install <MO2 directory>`, `--instance <instance name>` or
`--pid <process ID>`. The flags combine, and calls fail while more than one
process matches. After restart, the bridge discovers the new process session.
The bridge polls the selected process and sends the MCP client
`notifications/tools/list_changed` when its tools change or it switches to
another process.

## Direct connections

For a direct session, connect an MCP client to `http://127.0.0.1:<port>/mcp`.
HTTP scripts use the same tool names and arguments. See
[transport](transport.md) for endpoints, process discovery and errors.

```http
POST /api/tool/mods
Content-Type: application/json

{"action":"list"}
```

```json
{ "action": "createSeparator", "name": "Textures" }
```

```json
{ "action": "move", "name": "My Textures", "separator": "Textures_separator" }
```

```http
POST /api/tool/install
Content-Type: application/json

{"action":"start","path":"C:/Archives/Textures.zip","separator":"Textures_separator"}
```

## Tools

Read `/api/tools` for the complete current schemas or the generated
[tool reference](tools.md). [Action arguments](actions.md) covers common
workflows. Each tool groups related operations under `action`:

| Tool        | Operations                                                                                                            |
| ----------- | --------------------------------------------------------------------------------------------------------------------- |
| inspect     | Health, process identity and bridge setup                                                                             |
| mods        | List/detail, separators, creation/removal, rename, enablement, priority, separator placement, metadata and categories |
| categories  | Native category definitions, hierarchy, order and Nexus mappings                                                      |
| conflicts   | Per-mod file conflicts, overwrite winners, physical files and hiding                                                  |
| plugins     | Complete enabled/disabled plugin list, flags, masters, origins, enablement and ordering                               |
| executables | Native launch-target creation, editing, cloning, ordering, profile library rules and VFS launches                     |
| downloads   | Archives and metadata, URL/Nexus downloads, download path lookup                                                      |
| install     | Native archive installation sessions and completion state                                                             |
| dialogs     | FOMOD descriptions/options, file selection and native modal control                                                   |
| profiles    | Profile creation, copying, renaming, removal, selection, local saves/INIs and save transfer                           |
| files       | Virtual files, directories, resolution and origins                                                                    |
| settings    | Native MO2 settings sections, scoped plugin settings and refresh                                                      |
| nexus       | Mod descriptions, file details, download URLs and cached metadata                                                     |
| capture     | MO2 window and provider captures with image results and golden SSIM scoring                                           |
| logs        | Log source discovery and configurable tail reads                                                                      |
| events      | Sequenced semantic events                                                                                             |
| scenario    | Sequential tool calls and event waits                                                                                 |
| ui          | Native menu and toolbar action discovery and invocation                                                               |
