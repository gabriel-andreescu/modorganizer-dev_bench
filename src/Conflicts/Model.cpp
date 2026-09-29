#include "Conflicts/Model.h"
#include "Json.h"
#include "Mods/InfoAccess.h"
#include "Tools/Registry.h"
#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QDialog>
#include <QDir>
#include <QFont>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QString>
#include <QTabWidget>
#include <QTreeView>
#include <algorithm>
#include <format>
#include <map>
#include <qnamespace.h>
#include <string>
#include <utility>

namespace Bench {
namespace {
    struct ViewDefinition {
        const char* tree;
        const char* filter;
        int fileColumn;
    };

    const ViewDefinition& Definition(const Json& a_arguments) {
        static const std::map<std::string, ViewDefinition> views {
            {"winning", {.tree = "overwriteTree", .filter = "overwriteLineEdit", .fileColumn = 0}},
            {"losing", {.tree = "overwrittenTree", .filter = "overwrittenLineEdit", .fileColumn = 0}},
            {"unique", {.tree = "noConflictTree", .filter = "noConflictLineEdit", .fileColumn = 0}},
            {"advanced", {.tree = "conflictsAdvancedList", .filter = "conflictsAdvancedFilter", .fileColumn = 1}},
        };

        const auto view = a_arguments.value("view", "advanced");
        const auto found = views.find(view);
        if (found == views.end()) {
            throw ToolError(400, std::format("Unknown conflict view: {}", view));
        }
        return found->second;
    }

    QString RelativePath(QString a_path) {
        a_path = QDir::fromNativeSeparators(a_path);
        while (a_path.startsWith('/')) {
            a_path.remove(0, 1);
        }
        return a_path;
    }

    void SetOption(const QDialog* a_dialog, const Json& a_options, const char* a_key, const char* a_control) {
        if (a_options.contains(a_key)) {
            auto* button = a_dialog->findChild<QAbstractButton*>(a_control);
            if (button->isChecked() != a_options.at(a_key).get<bool>()) {
                button->click();
            }
        }
    }
}

QTreeView* ConflictView(const Json& a_arguments) {
    const auto* dialog = RequireModInfo();
    SelectModInfoTab(dialog, "tabConflicts");
    const auto& definition = Definition(a_arguments);
    dialog->findChild<QTabWidget*>("tabConflictsTabs")->setCurrentIndex(definition.fileColumn == 1 ? 1 : 0);

    const auto options = a_arguments.value("options", Json::object());
    SetOption(dialog, options, "includeUnique", "conflictsAdvancedShowNoConflict");
    if (options.contains("allProviders")) {
        const auto* control = options.at("allProviders").get<bool>() ? "conflictsAdvancedShowAll"
                                                                     : "conflictsAdvancedShowNearest";
        dialog->findChild<QAbstractButton*>(control)->click();
    }
    if (a_arguments.contains("filter")) {
        dialog->findChild<QLineEdit*>(definition.filter)->setText(Text(a_arguments.at("filter")));
    }
    return dialog->findChild<QTreeView*>(definition.tree);
}

Json ReadConflicts(const Json& a_arguments) {
    const auto* tree = ConflictView(a_arguments);
    const auto* model = tree->model();
    const auto& definition = Definition(a_arguments);
    const auto offset = a_arguments.value("offset", 0);
    const auto limit = a_arguments.value("limit", 100);
    const auto total = model->rowCount();
    const auto end = offset >= total ? total : offset + std::min(limit, total - offset);
    Json rows = Json::array();
    for (int row = offset; row < end; ++row) {
        const auto index = model->index(row, definition.fileColumn);
        Json entry = {
            {"path", Utf8(RelativePath(index.data().toString()))},
            {"archive", index.data(Qt::FontRole).value<QFont>().italic()},
        };
        if (definition.fileColumn == 1) {
            entry["overwritesText"] = Utf8(model->index(row, 0).data().toString());
            entry["overwrittenByText"] = Utf8(model->index(row, 2).data().toString());
        } else if (model->columnCount() == 2) {
            entry[a_arguments.value("view", "advanced") == "winning" ? "overwritesText" : "overwrittenByText"] = Utf8(
                model->index(row, 1).data().toString()
            );
        }
        rows.push_back(std::move(entry));
    }

    const auto* dialog = RequireModInfo();
    return {
        {"name", Utf8(dialog->windowTitle())},
        {"view", a_arguments.value("view", "advanced")},
        {"filter", Utf8(dialog->findChild<QLineEdit*>(definition.filter)->text())},
        {
            "options",
            {
                {"includeUnique", dialog->findChild<QAbstractButton*>("conflictsAdvancedShowNoConflict")->isChecked()},
                {"allProviders", dialog->findChild<QAbstractButton*>("conflictsAdvancedShowAll")->isChecked()},
            },
        },
        {"total", total},
        {"offset", offset},
        {"nextOffset", end < total ? Json(end) : Json(nullptr)},
        {"files", rows},
    };
}

void SelectConflictFiles(const QTreeView* a_tree, const Json& a_paths) {
    if (a_paths.empty()) {
        throw ToolError(400, "Select at least one file path");
    }
    const auto* model = a_tree->model();
    const auto column = a_tree->objectName() == "conflictsAdvancedList" ? 1 : 0;
    QItemSelection selection;
    for (const auto& path : a_paths) {
        QModelIndex found;
        for (int row = 0; row < model->rowCount(); ++row) {
            const auto index = model->index(row, column);
            if (RelativePath(index.data().toString()).compare(Text(path), Qt::CaseInsensitive) == 0) {
                found = index;
                break;
            }
        }
        if (!found.isValid()) {
            throw ToolError(404, "File is not in the current conflict view: " + path.get<std::string>());
        }
        selection.select(found, found);
    }
    a_tree->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}
}
