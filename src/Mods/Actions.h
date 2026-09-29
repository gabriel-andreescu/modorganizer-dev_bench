#pragma once
#include "Json.h"
#include "Native/Operation.h"
#include <uibase/imoinfo.h>

namespace Bench {
class ModActions : public NativeOperation {
public:
    ModActions(MOBase::IOrganizer* a_organizer, EventBus& a_events, QObject* a_parent)
        : NativeOperation(a_events, a_parent)
        , _organizer(a_organizer) {}

    Json Invoke(const Json& a_args);
    [[nodiscard]] Json Describe(const QString& a_name) const;
    void Move(const QString& a_name, int a_priority) const;
    void UnderSeparator(const QString& a_name, const QString& a_separator) const;

private:
    [[nodiscard]] Json List(bool a_separatorsOnly) const;
    Json InvokeMod(const Json& a_args);
    [[nodiscard]] MOBase::IModInterface* Require(const QString& a_name) const;
    Json Create(const QString& a_requested, bool a_separator);
    Json Metadata(const Json& a_arguments);
    Json Remove(const QString& a_name);
    Json MenuOperation(const Json& a_arguments);
    MOBase::IOrganizer* _organizer;
};
}
