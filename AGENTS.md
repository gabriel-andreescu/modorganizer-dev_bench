# AGENTS.md

Dev Bench gives MCP and REST clients access to a running MO2. Use the
[documentation index](docs/README.md) for user and plugin-author guides. Read
[development](docs/maintainers/development.md) for building, tests and source
layout.

## Code

- Build and package MO2 2.5.2 and MO2 2.5.3beta12 sequentially. Never substitute
  current MO2 master for a supported release.
- Use C++23, the DLL MSVC runtime, warnings as errors and the repository
  clang-format style. Run clang-format on every changed C++ file, then
  clang-tidy for each MO2 release.
- Keep each file and function responsible for one operation or cohesive
  abstraction, and group a module's files in a directory named for it. Comments
  explain constraints, not syntax. File/function length is not a design metric.
- Tool descriptors are the source of truth for MCP, REST, the offline bridge
  catalog and generated reference. Keep adapters free of domain-specific
  dispatch. Marshal MO2 and Qt widget access onto the UI thread.
- Runtime discovery must verify process session identity before invoking
  anything. Several MO2 processes may run Dev Bench at once. The bridge selects
  one by install, instance or PID and fails while the selection is ambiguous.
  MO2 is launched externally.
- Drive installation through MO2's installer pipeline. Preserve native FOMOD
  validation and conditional pages. Report asynchronous session state honestly.
  Never claim queued work has completed.

## Validation

- Prefer unit tests for pure logic, transport integration tests for protocol and
  process discovery, and a small opt-in live MO2 suite. Tests verify the
  supported public contract and observable outcomes.
- Put temporary probes in the ignored `scratch/` directory. Do not interrupt
  unrelated MO2 processes.

## Documentation

- README is the project front page. Guides live under `docs/`, grouped by
  audience and indexed by `docs/README.md`. Document each fact in one place and
  link to it.
- Keep private paths and task discussion out of published files. Do not commit
  or publish unless requested.
