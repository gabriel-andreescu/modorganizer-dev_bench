#include "VariantJson.h"
#include "Json.h"
#include <QJsonDocument>
#include <QVariant>

namespace Bench {
Json VariantJson(const QVariant& a_value) {
    const auto document = QJsonDocument::fromVariant(QVariantList {a_value});
    return Json::parse(document.toJson(QJsonDocument::Compact).toStdString()).front();
}
}
