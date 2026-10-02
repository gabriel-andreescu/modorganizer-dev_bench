package("cpp-mcp")
set_homepage("https://github.com/hkr04/cpp-mcp")
set_description("Lightweight C++ MCP (Model Context Protocol) SDK")
set_license("MIT")
add_urls("https://github.com/hkr04/cpp-mcp.git")
add_versions("2026.07.15", "f1117d5286efe6477ddd14322703f034615b6c0e")
-- The REST adapter mounts its routes on the MCP server's HTTP listener.
add_patches(
    "2026.07.15",
    path.join(os.scriptdir(), "patches", "http-server-access.patch"),
    "7e42d4d0998facf44547910065ad2b840d51bc289f99bc64b094e92047b6ec21"
)
-- The host binds the listener with an exclusive socket before starting the server.
add_patches(
    "2026.07.15",
    path.join(os.scriptdir(), "patches", "listen-after-bind.patch"),
    "ea136456099d694725bdad5b7cf56ddbe5a393190d4a108b7bc522823f34c00d"
)
add_defines("MCP_MAX_SESSIONS=10", "MCP_SESSION_TIMEOUT=30")
add_syslinks("ws2_32", "wsock32")
on_install("windows", function(package)
    io.writefile(
        "xmake.lua",
        [[
add_rules("mode.debug", "mode.release")
target("mcp")
    set_kind("static")
    set_languages("c++17")
    add_files("src/*.cpp")
    add_includedirs("include", "common")
    add_headerfiles("include/*.h", "common/*.h", "common/*.hpp")
    add_defines("MCP_MAX_SESSIONS=10", "MCP_SESSION_TIMEOUT=30", "_CRT_SECURE_NO_WARNINGS")
    add_cxflags("/utf-8", "/bigobj", { tools = "cl" })
]]
    )
    import("package.tools.xmake").install(package)
end)
on_test(function(package)
    assert(package:check_cxxsnippets({
        test = [[
            #include <mcp_server.h>
            void test() {
                mcp::server::configuration config;
                mcp::server server(config);
                server.http()->listen_after_bind();
            }
        ]],
    }, { configs = { languages = "c++17" } }))
end)
