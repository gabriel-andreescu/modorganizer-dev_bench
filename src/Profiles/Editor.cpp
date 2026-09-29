#include "Profiles/Editor.h"
#include "Json.h"
#include <QCheckBox>
#include <QListWidget>
#include <QObject>

namespace Bench {
Json ReadProfileEditor(const QWidget* a_dialog) {
    const auto* list = a_dialog->findChild<QListWidget*>("profilesList");
    Json names = Json::array();
    for (int row = 0; row < list->count(); ++row) {
        names.push_back(Utf8(list->item(row)->text()));
    }
    Json selected = nullptr;
    if (const auto* item = list->currentItem()) {
        selected = {
            {"name", Utf8(item->text())},
            {"localSaves", a_dialog->findChild<QCheckBox*>("localSavesBox")->isChecked()},
            {"localSettings", a_dialog->findChild<QCheckBox*>("localIniFilesBox")->isChecked()},
        };
    }
    return {{"profiles", names}, {"selected", selected}, {"dialogOpen", a_dialog->isVisible()}};
}
}
