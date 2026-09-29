#pragma once
#include "Json.h"
#include <functional>
#include <map>
#include <mutex>

namespace Bench {
class ToolError : public std::runtime_error {
public:
    ToolError(int a_status, const std::string& a_message)
        : std::runtime_error(a_message)
        , _code(a_status) {}

    [[nodiscard]] int Code() const noexcept {
        return _code;
    }

private:
    int _code;
};

class ToolRegistry {
public:
    using Handler = std::function<Json(const Json&)>;

    // Returns false when the registration replaced an existing tool.
    bool Add(Json a_descriptor, Handler a_handler);
    Json List() const;
    Json Invoke(const std::string& a_name, const Json& a_arguments) const;
    void OnChanged(std::function<void(const Json&)> a_callback);

private:
    struct Entry {
        Json descriptor;
        Handler handler;
    };

    mutable std::mutex _mutex;
    std::map<std::string, Entry> _entries;
    std::function<void(const Json&)> _changed;
};

void Validate(const Json& a_schema, const Json& a_value, const std::string& a_path = "arguments");
}
