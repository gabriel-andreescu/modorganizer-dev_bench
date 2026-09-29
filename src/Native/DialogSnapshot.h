#pragma once
#include "Json.h"
#include <QString>
#include <functional>
class QDialog;

namespace Bench {
Json SnapshotNativeDialog(
    const QString& a_className,
    const std::function<void()>& a_open,
    std::function<Json(QDialog*)> a_read
);
}
