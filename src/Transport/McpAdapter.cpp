#include "Json.h"
#include "Tools/Registry.h"
#include "Transport/Server.h"
#include <exception>
#include <string>

namespace Bench {
void Server::MountMcp() {
    _server->register_method("tools/list", [this](const Json&, const std::string&) {
        auto tools = _registry.List();
        for (auto& tool : tools) {
            tool["annotations"] = {{"readOnlyHint", tool.value("readOnly", false)}};
            tool.erase("readOnly");
        }
        return Json {{"tools", tools}};
    });
    _server->register_method("tools/call", [this](const Json& a_params, const std::string&) {
        Json result;
        std::string text;
        try {
            auto value = _registry.Invoke(
                a_params.at("name").get<std::string>(),
                a_params.value("arguments", Json::object())
            );
            if (value.contains("content") && value["content"].is_array()) {
                return value;
            }
            text = value.dump();
            result["isError"] = false;
        } catch (const std::exception& error) {
            text = error.what();
            result["isError"] = true;
        }
        result["content"] = Json::array({{{"type", "text"}, {"text", text}}});
        return result;
    });
}
}
