#pragma once
#include "Native/DialogWorkflow.h"

namespace Bench {
class CategoryManager : public NativeDialogWorkflow {
public:
    using NativeDialogWorkflow::NativeDialogWorkflow;
    Json Invoke(const Json& a_arguments);
};
}
