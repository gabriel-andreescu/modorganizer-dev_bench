#include "Mods/Editor.h"
#include "Json.h"
#include "Mods/InfoAccess.h"
#include "Tools/Registry.h"
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QObject>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVariant>
#include <format>
#include <qnamespace.h>
#include <set>
#include <string>

namespace Bench {
namespace {
    Json ReadEditor(const QDialog* a_dialog) {
        auto* tree = a_dialog->findChild<QTreeWidget*>("categories");
        Json categories = Json::array();
        for (QTreeWidgetItemIterator iterator(tree); (*iterator) != nullptr; ++iterator) {
            const auto* item = *iterator;
            categories.push_back({
                {"id", item->data(0, Qt::UserRole).toInt()},
                {"name", Utf8(item->text(0))},
                {
                    "parentId",
                    (item->parent() != nullptr) ? Json(item->parent()->data(0, Qt::UserRole).toInt()) : Json(nullptr),
                },
                {"assigned", item->checkState(0) == Qt::Checked},
            });
        }
        const auto* primary = a_dialog->findChild<QComboBox*>("primaryCategories");
        const auto* notes = a_dialog->findChild<QTextEdit*>("notes");
        return {
            {"name", Utf8(a_dialog->windowTitle())},
            {"categories", categories},
            {"primaryCategory", primary->currentIndex() < 0 ? Json(nullptr) : Json(primary->currentData().toInt())},
            {"comments", Utf8(a_dialog->findChild<QLineEdit*>("comments")->text())},
            {"notes", Utf8(notes->toPlainText().isEmpty() ? QString() : notes->toHtml())},
        };
    }

    QTreeWidgetItem* FindCategory(QTreeWidget* a_tree, int a_identifier) {
        for (QTreeWidgetItemIterator iterator(a_tree); (*iterator) != nullptr; ++iterator) {
            if ((*iterator)->data(0, Qt::UserRole).toInt() == a_identifier) {
                return *iterator;
            }
        }
        throw ToolError(404, std::format("Unknown category ID: {}. Read editorState", a_identifier));
    }

    void AssignCategories(const QDialog* a_dialog, const Json& a_entries) {
        auto* tree = a_dialog->findChild<QTreeWidget*>("categories");
        for (const auto& entry : a_entries) {
            FindCategory(tree, entry.at("id").get<int>())
                ->setCheckState(0, entry.at("assigned").get<bool>() ? Qt::Checked : Qt::Unchecked);
        }
    }

    void SelectPrimaryCategory(const QDialog* a_dialog, int a_identifier) {
        auto* combo = a_dialog->findChild<QComboBox*>("primaryCategories");
        const auto index = combo->findData(a_identifier);
        if (index < 0) {
            throw ToolError(409, "Primary category must be assigned to the mod");
        }
        combo->setCurrentIndex(index);
    }

    void UpdateCategories(const QDialog* a_dialog, const Json& a_values) {
        SelectModInfoTab(a_dialog, "tabCategories");
        if (a_values.contains("categories")) {
            AssignCategories(a_dialog, a_values.at("categories"));
        }
        if (a_values.contains("primaryCategory")) {
            SelectPrimaryCategory(a_dialog, a_values.at("primaryCategory").get<int>());
        }
    }

    void UpdateNotes(const QDialog* a_dialog, const Json& a_values) {
        SelectModInfoTab(a_dialog, "tabNotes");
        if (a_values.contains("comments")) {
            auto* comments = a_dialog->findChild<QLineEdit*>("comments");
            comments->setText(Text(a_values.at("comments")));
            QMetaObject::invokeMethod(comments, "editingFinished", Qt::DirectConnection);
        }
        if (a_values.contains("notes")) {
            auto* notes = a_dialog->findChild<QTextEdit*>("notes");
            notes->setHtml(Text(a_values.at("notes")));
            QMetaObject::invokeMethod(notes, "editingFinished", Qt::DirectConnection);
        }
    }
}

Json ModEditor(const Json& a_arguments) {
    auto* dialog = RequireModInfo();
    const auto& action = a_arguments.at("action");
    if (action == "closeEditor") {
        QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
        return {{"scheduled", true}};
    }
    if (action == "editorUpdate") {
        const auto& values = a_arguments.at("values");
        const std::set<std::string> fields {"comments", "notes", "categories", "primaryCategory"};
        for (const auto& [key, value] : values.items()) {
            if (!fields.contains(key)) {
                throw ToolError(400, "Unknown mod editor field: " + key);
            }
        }
        if (values.contains("comments") || values.contains("notes")) {
            UpdateNotes(dialog, values);
        }
        if (values.contains("categories") || values.contains("primaryCategory")) {
            UpdateCategories(dialog, values);
        }
    }
    auto* tabs = dialog->findChild<QTabWidget*>("tabWidget");
    const auto selected = tabs->currentIndex();
    SelectModInfoTab(dialog, "tabCategories");
    SelectModInfoTab(dialog, "tabNotes");
    auto result = ReadEditor(dialog);
    tabs->setCurrentIndex(selected);
    return result;
}
}
