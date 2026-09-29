#pragma once
#include <QString>
#include <QStringList>
#include <json.hpp>

namespace Bench {
using Json = nlohmann::ordered_json;

inline std::string Utf8(const QString& a_value) {
    return a_value.toUtf8().toStdString();
}

inline QString Text(const Json& a_value) {
    return QString::fromStdString(a_value.get<std::string>());
}

inline Json Strings(const QStringList& a_values) {
    Json result = Json::array();
    for (const auto& value : a_values) {
        result.push_back(Utf8(value));
    }
    return result;
}

inline QStringList StringList(const Json& a_values) {
    QStringList result;
    for (const auto& value : a_values) {
        result.push_back(Text(value));
    }
    return result;
}
}
