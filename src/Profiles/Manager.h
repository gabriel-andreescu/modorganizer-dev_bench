#pragma once

#include "Json.h"
#include "Native/DialogWorkflow.h"
#include <QComboBox>

namespace Bench {
class ProfileManager : public NativeDialogWorkflow {
public:
    using NativeDialogWorkflow::NativeDialogWorkflow;
    Json Invoke(const Json& a_args, QComboBox* a_picker);
};
}
