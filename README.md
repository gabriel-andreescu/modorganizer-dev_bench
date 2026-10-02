# modorganizer-dev_bench

MO2 plugin providing MCP and REST access to mod management, native installation,
load order, profiles, downloads, virtual files and application launches.

[Install the plugin and connect a client](docs/automation/getting-started.md),
or [register tools from your own plugin](docs/plugin-authors/extensions.md). See
the [documentation](docs/README.md) for tool workflows and the transport
contract.

## Design your tools the agentic-renderdoc way

Dev Bench follows the
**[agentic-renderdoc](https://github.com/EdenLabs/agentic-renderdoc#why-this-design)**
model: a _thin but powerful_ surface an agent can drive, where a call **returns
the value**, not just an ack. Match that when you register, so an agent gets a
coherent bench rather than a pile of one-off verbs:

- **Return data, not "ok."** A read should answer the question
  (`{ "enabled": true, "loadOrder": 81 }`), not `{ "queued": true }`. Dev Bench
  runs your handler on MO2's UI thread, so return the result synchronously (Dev
  Bench's own `plugins` does this).
- **Few powerful tools over many narrow ones.** Prefer one `plugins` tool with an
  `action` enum to four verbs. A general primitive (an `eval`-style entry into
  your subsystem) beats a tool per operation.
- **Self-describe.** Put a real `inputSchema` and a clear `description` on every
  tool — that _is_ the MCP schema and the REST docs. It's how an agent discovers
  what you offer cold.
- **Make failure legible.** Validate inputs and return an actionable error (what
  was wrong + how to list valid values), rather than silently succeeding.

## Development

See [development setup and tests](docs/maintainers/development.md).

## License

[GPL-3.0-or-later](COPYING) WITH
[Modding Exception AND GPL-3.0 Linking Exception (with Corresponding Source)](EXCEPTIONS).
The Modded Code is Mod Organizer 2. Modding Libraries include MO2's uibase, Qt
and Windows. See [dependencies](docs/dependencies.md) for bundled and
third-party components.

The cross-plugin **API glue is separately MIT**: `include/DevBenchAPI.h`,
`include/DevBenchAPI.cpp`, `include/DevBenchQt.h` and
`include/DevBenchAPI.LICENSE.txt`. MO2 plugins may vendor those files or use the
`devbench-api` package without adopting Dev Bench's GPL license.
