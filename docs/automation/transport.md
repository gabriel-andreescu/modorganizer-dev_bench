# Transport contract

The listener binds only to 127.0.0.1. It has no authentication, and any local
process can call its tools, which remove mods and launch executables. Never
expose the port on a network. Default port: 8930, falling back to the next
available port. The configured port is a preference. Several MO2 processes can
run Dev Bench at once, each on its own port. Each process writes
`%LOCALAPPDATA%/devbench/mo2/instances/<pid>.json` with the actual bound port,
MO2 install directory, instance name, managed game name, executable, PID and a
UUID unique to the current process session, and removes it on exit. The instance
name is empty for portable installs. Health reports the same identity.

- `POST /mcp`: streamable HTTP MCP, using cpp-mcp.
- `GET /api/tools`: descriptors, JSON schemas and the bridge setup snippet.
- `POST /api/tool/<name>`: JSON arguments and a JSON result.
- `GET /api/health`: identity and UI heartbeat, pending/completed task counters.
- `GET /api/events?since=N`: event history with sequence cursor and gap
  reporting. Send `Accept: text/event-stream` for SSE.

The bridge reads the records on every call and ignores those left by crashes
when health no longer matches. It verifies the selected process's identity
through health and supplies `X-Dev-Bench-Session`. A mismatch returns HTTP 409.
Direct HTTP clients should send the same header when commands must stay pinned
to a process, so a recycled port never receives a command meant for an earlier
MO2.

Errors use HTTP 400 for invalid input, 404 for missing objects, 409 for
conflicts, 503 during shutdown and 504 when the UI task deadline expires. A task
that has not started is cancelled on timeout. A task already executing may still
finish, so inspect state before retrying a mutation.

Registry changes publish a `tools.changed` event with the tool name, so clients
polling `/api/events` can refresh their catalog.

Health never waits for the Qt event loop. A stopped heartbeat indicates that Qt
is not servicing the heartbeat timer. It does not establish a crash by itself.
