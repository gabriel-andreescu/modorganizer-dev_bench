#pragma once
#include "Json.h"
#include <QString>

namespace Bench {
class Runtime {
public:
    // An empty instance name identifies a portable MO2 install.
    Runtime(const QString& a_instance, const QString& a_game);
    void Publish(int a_port);
    void Remove() const;
    [[nodiscard]] Json Identity() const;
    [[nodiscard]] Json Bridge() const;

private:
    Json _identity;
    QString _record;
};
}
