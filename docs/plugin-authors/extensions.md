# Registering tools and capture providers

`include/DevBenchAPI.h` and its companion `DevBenchAPI.cpp` are MIT-licensed.
Compile the companion source into your plugin, or use the MOPK `devbench-api`
package and its integration rule. Call `DevBenchAPI::GetDevBenchInterface001()`
after MO2's user interface is initialized, once every
`onUserInterfaceInitialized` callback has run, for example from a zero-delay
timer started in that callback. It returns `nullptr` when Dev Bench is not
loaded.

## Tools

`RegisterTool(name, descriptorJson, handler, context)` adds a tool. The
descriptor contains `description`, `inputSchema` and optionally `readOnly`.
Registering an existing name replaces that tool and returns `false`. Namespace
tool names and event topics with your plugin's name.

## Capture providers

`RegisterToolExtension("capture", key, descriptorJson, handler, context)` adds a
capture provider. `capture` is the only base tool that uses extensions.
Registering an existing key replaces that provider and returns `false`. The key
joins `capture`'s `kind` enum, and `kind=extensions` lists each provider with
its descriptor.

`capture kind=<key>` passes the caller's arguments to the handler, plus
`outputPath` and `requestId`. The provider writes a PNG to `outputPath`. It can
then emit `capture.ready` with `requestId`, `ok: true` and optionally `width`,
`height` and `uiExcluded`. Dev Bench also polls the file, so the event is
optional. A handler result containing `error` fails the capture with that
message. Dev Bench then publishes the image, scores it against a golden when
asked and reports it as described in
[Captures](../automation/actions.md#captures).

`RegisterMenuHandler` stores its handler and returns `true`, or `false` when it
replaces one. Dev Bench has no `menu` tool, so the handler is never invoked.

## Handlers

Handlers receive a UTF-8 JSON arguments object and write one UTF-8 JSON result
through the host's `WriteFn` before returning. Handlers run one at a time on
MO2's UI thread, queued with Dev Bench's own UI work, so they can use MO2 and Qt
widgets directly. A call that has not finished after 30 seconds fails, and is
cancelled if it had not started.

The handler and context must remain valid until MO2 shuts down. Do not unload a
module with registered handlers. `EmitEvent` publishes a JSON value on the
shared event bus. These functions do not permit exceptions across the DLL
boundary.

`include/DevBenchQt.h`, MIT-licensed like the API, adapts handlers to Qt JSON.
`DevBenchQt::RespondToTool` and `DevBenchQt::RespondToExtension` parse the
arguments into a `QJsonObject`, call a `QJsonObject(const QJsonObject&)` handler
and write its result. Malformed arguments and exceptions become an MCP error
result for tools and `{"error": message}` for capture providers:

```cpp
void Invoke(void* a_ctx, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write) {
    DevBenchQt::RespondToTool(
        [a_ctx](const QJsonObject& a_arguments) { return static_cast<MyTool*>(a_ctx)->Handle(a_arguments); },
        a_argsJson,
        a_sink,
        a_write
    );
}
```

The input validator supports the schema constructs used by the core catalog:
object properties, required and additionalProperties booleans, primitive types,
arrays and items, enum and const, numeric minimum, anyOf, and allOf with if/then
conditions. Validate any additional constraints in the handler.

For rich output, write an MCP result object with a `content` array and optional
`structuredContent` or `isError`. Both MCP adapters preserve this envelope,
including image blocks with base64 `data` and `mimeType`. HTTP returns the same
JSON envelope. Ordinary JSON results are wrapped in one MCP text block. Reserve
the top-level `content` array for this rich-result contract.
