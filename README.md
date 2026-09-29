# modorganizer-dev_bench

MO2 plugin providing MCP and REST access to mod management, native installation,
load order, profiles, downloads, virtual files and application launches.

[Install the plugin and connect a client](docs/automation/getting-started.md),
or [register tools from your own plugin](docs/plugin-authors/extensions.md). See
the [documentation](docs/README.md) for tool workflows and the transport
contract.

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
