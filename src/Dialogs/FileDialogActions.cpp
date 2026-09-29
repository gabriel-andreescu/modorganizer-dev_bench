#include "Dialogs/FileDialogActions.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QObject>
#include <QWidget>
#include <map>

namespace Bench {
Json DescribeFileDialog(const QFileDialog* a_dialog) {
    const std::map<QFileDialog::FileMode, const char*> modes {
        {QFileDialog::AnyFile, "anyFile"},
        {QFileDialog::ExistingFile, "existingFile"},
        {QFileDialog::Directory, "directory"},
        {QFileDialog::ExistingFiles, "existingFiles"},
    };
    return {
        {"directory", Utf8(a_dialog->directory().absolutePath())},
        {"selectedFiles", Strings(a_dialog->selectedFiles())},
        {"nameFilters", Strings(a_dialog->nameFilters())},
        {"selectedNameFilter", Utf8(a_dialog->selectedNameFilter())},
        {"fileMode", modes.at(a_dialog->fileMode())},
        {"acceptMode", a_dialog->acceptMode() == QFileDialog::AcceptOpen ? "open" : "save"},
    };
}

void SelectDialogFile(QWidget* a_widget, const Json& a_args) {
    auto* dialog = qobject_cast<QFileDialog*>(a_widget);
    if (dialog == nullptr) {
        throw ToolError(400, "Use a file dialog id");
    }
    if (dialog->findChild<QLineEdit*>("fileNameEdit") == nullptr) {
        throw ToolError(409, "File selection is unavailable in this native dialog");
    }
    if (auto* focused = dialog->focusWidget()) {
        focused->clearFocus();
    }
    dialog->selectFile(Text(a_args.at("path")));
}
}
