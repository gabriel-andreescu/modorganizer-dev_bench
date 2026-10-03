# Dependencies

- MO2 uibase headers: LGPL-3.0-or-later, from each supported MO2 release.
- Qt: linked dynamically through the MO2 host.
- cpp-mcp: MIT, pinned and patched in packages/c/cpp-mcp. Its bundled
  cpp-httplib and nlohmann JSON headers retain their MIT notices.
- MCP TypeScript SDK: MIT, locked in bridge/package-lock.json.
- Bun: standalone bridge compiler/runtime, MIT. It statically links
  JavaScriptCore, which is LGPL-2.

The `licenses` directory beside this documentation holds these notices and those
of the bridge's npm dependencies.
