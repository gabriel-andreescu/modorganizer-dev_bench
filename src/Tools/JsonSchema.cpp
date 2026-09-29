#include "Json.h"
#include "Tools/Registry.h"
#include <algorithm>
#include <format>
#include <string>

namespace Bench {
namespace {
    bool IsType(const Json& a_value, const std::string& a_type) {
        if (a_type == "object") {
            return a_value.is_object();
        }
        if (a_type == "array") {
            return a_value.is_array();
        }
        if (a_type == "string") {
            return a_value.is_string();
        }
        if (a_type == "boolean") {
            return a_value.is_boolean();
        }
        if (a_type == "integer") {
            return a_value.is_number_integer();
        }
        if (a_type == "number") {
            return a_value.is_number();
        }
        return a_type.empty();
    }

    void ObjectFields(const Json& a_schema, const Json& a_value, const std::string& a_path) {
        for (const auto& key : a_schema.value("required", Json::array())) {
            if (!a_value.contains(key.get<std::string>())) {
                throw ToolError(400, a_path + " requires " + key.get<std::string>());
            }
        }
        const auto properties = a_schema.value("properties", Json::object());
        for (const auto& [key, item] : a_value.items()) {
            if (properties.contains(key)) {
                auto child = a_path;
                child += '.';
                child += key;
                Validate(properties[key], item, child);
            } else if (!a_schema.value("additionalProperties", true)) {
                auto message = a_path;
                message += " has unknown field ";
                message += key;
                throw ToolError(400, message);
            }
        }
    }

    void Conditions(const Json& a_schema, const Json& a_value, const std::string& a_path) {
        if (a_schema.contains("anyOf")) {
            bool matched = false;
            for (const auto& alternative : a_schema.at("anyOf")) {
                try {
                    Validate(alternative, a_value, a_path);
                    matched = true;
                    break;
                } catch (const ToolError&) {
                    matched = false;
                }
            }
            if (!matched) {
                throw ToolError(400, a_path + " does not match an available action schema");
            }
        }
        for (const auto& rule : a_schema.value("allOf", Json::array())) {
            bool matches = true;
            if (rule.contains("if")) {
                try {
                    Validate(rule["if"], a_value, a_path);
                } catch (const ToolError&) {
                    matches = false;
                }
            }
            if (matches) {
                Validate(rule.contains("then") ? rule["then"] : rule, a_value, a_path);
            }
        }
    }
}

void Validate(const Json& a_schema, const Json& a_value, const std::string& a_path) {
    const auto type = a_schema.value("type", std::string());
    if (!IsType(a_value, type)) {
        throw ToolError(400, a_path + " must be " + type);
    }
    if (a_schema.contains("const") && a_schema["const"] != a_value) {
        throw ToolError(400, a_path + " must equal " + a_schema["const"].dump());
    }
    if (a_schema.contains("enum") && std::ranges::find(a_schema["enum"], a_value) == a_schema["enum"].end()) {
        throw ToolError(400, a_path + " must be one of " + a_schema["enum"].dump());
    }
    if (a_value.is_object()) {
        ObjectFields(a_schema, a_value, a_path);
    }
    if (a_value.is_array() && a_schema.contains("items")) {
        for (const auto& item : a_value) {
            Validate(a_schema["items"], item, a_path + "[]");
        }
    }
    if (a_value.is_number()
        && a_schema.contains("minimum")
        && a_value.get<double>() < a_schema["minimum"].get<double>()) {
        throw ToolError(400, std::format("{} is below minimum {}", a_path, a_schema["minimum"].dump()));
    }
    Conditions(a_schema, a_value, a_path);
}
}
