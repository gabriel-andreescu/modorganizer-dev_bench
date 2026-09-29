// SPDX-License-Identifier: MIT
//
// This interface header and its companion DevBenchAPI.cpp are MIT-licensed. Any
// MO2 plugin may vendor them or consume the devbench-api package.
#pragma once

// Dev Bench cross-plugin API. Lets another MO2 plugin register MCP/REST tools and
// emit events into the running Dev Bench host. Request the interface after MO2's
// user interface is initialized, once every onUserInterfaceInitialized callback
// has run.
//
// The consumer supplies callbacks that Dev Bench invokes for tool requests. JSON
// strings, plain C function pointers, and opaque context pointers keep C++ object
// ownership within each DLL.
namespace DevBenchAPI {
inline constexpr const char* kModuleName = "dev_bench.dll";
inline constexpr const char* kQueryInterfaceExport = "DevBenchQueryInterface";

using QueryInterfaceFn = void* (*)(unsigned int a_revision);

// A handler calls this host-owned writer exactly once before returning. Result
// string ownership stays within the host DLL.
using WriteFn = void (*)(void* a_sink, const char* a_resultJson);

// Dev Bench invokes tool handlers on MO2's UI thread, so handlers may use MO2 and Qt
// widgets directly. The handler must be a plain C function or captureless lambda.
using ToolFn = void (*)(void* a_ctx, const char* a_argsJson, void* a_sink, WriteFn a_write);

struct IDevBenchInterface001;
// Returns nullptr if Dev Bench is absent or revision 1 is unavailable.
IDevBenchInterface001* GetDevBenchInterface001();

struct IDevBenchInterface001 {
    // Dev Bench build number: VERSION_MAJOR*10000 + MINOR*100 + PATCH.
    virtual unsigned int GetBuildNumber() = 0;

    // Register a tool, exposed over both MCP (/mcp) and REST (/api/tool/<name>).
    // a_descriptorJson: { "description": str, "inputSchema": obj, "readOnly": bool }.
    // Returns false if a tool of this name already existed (it is still replaced).
    virtual bool RegisterTool(const char* a_name, const char* a_descriptorJson, ToolFn a_handler, void* a_ctx) = 0;

    // Publish an event (MCP notification + REST /api/events?since=N).
    virtual void EmitEvent(const char* a_topic, const char* a_payloadJson) = 0;

    // Kept for API compatibility. Dev Bench has no menu tool, so the handler is stored
    // but never invoked. Returns false if it replaced an existing handler.
    virtual bool RegisterMenuHandler(
        const char* a_menuName,
        const char* a_descriptorJson,
        ToolFn a_handler,
        void* a_ctx
    ) = 0;

    // Register a keyed extension under an opted-in base tool. The base tool routes
    // requests and exposes the descriptor through its discovery action. The only
    // such base tool is capture. Returns false when it replaces an entry.
    // Capture providers write outputPath. For asynchronous completion, emit
    // capture.ready with requestId, ok:true, and optional width/height.
    // The host also polls the file. A callback result containing error reports failure.
    virtual bool RegisterToolExtension(
        const char* a_baseTool,
        const char* a_key,
        const char* a_descriptorJson,
        ToolFn a_handler,
        void* a_ctx
    ) = 0;
};
}
