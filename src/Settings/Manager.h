#pragma once

#include "Dialogs/Actions.h"
#include "Native/DialogWorkflow.h"

namespace Bench {
class SettingsManager : public NativeDialogWorkflow {
public:
    SettingsManager(DialogActions& a_dialogs, EventBus& a_events, QObject* a_parent);
    Json Invoke(const Json& a_args);

private:
    Json Operate(QWidget* a_dialog, const Json& a_args);
    DialogActions& _dialogs;
};
}
