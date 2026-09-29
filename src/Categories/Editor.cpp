#include "Categories/Editor.h"
#include "Json.h"
#include "Native/Menu.h"
#include "Tools/Registry.h"
#include <QAbstractItemDelegate>
#include <QCoreApplication>
#include <QDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaObject>
#include <QMimeData>
#include <QModelIndex>
#include <QObject>
#include <QPoint>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QVariant>
#include <format>
#include <map>
#include <memory>
#include <qnamespace.h>
#include <set>
#include <string>
#include <utility>

namespace Bench {
namespace {
    QTableWidgetItem* RequireRow(const QTableWidget* a_table, int a_row) {
        if (a_row < 0 || a_row >= a_table->rowCount()) {
            throw ToolError(
                400,
                std::format("Category row {} is out of range 0..{}. Read editorState", a_row, a_table->rowCount() - 1)
            );
        }
        return a_table->item(a_row, 0);
    }

    Json ReadCategories(const QWidget* a_dialog) {
        const auto* table = a_dialog->findChild<QTableWidget*>("categoriesTable");
        Json rows = Json::array();
        for (int visual = 0; visual < table->rowCount(); ++visual) {
            const auto row = table->verticalHeader()->logicalIndex(visual);
            Json mappings = Json::array();
            for (const auto& entry : table->item(row, 3)->data(Qt::UserRole).toList()) {
                const auto data = entry.toList();
                mappings.push_back({{"id", data.at(1).toInt()}, {"name", Utf8(data.at(0).toString())}});
            }
            rows.push_back({
                {"row", row},
                {"id", table->item(row, 0)->text().toInt()},
                {"name", Utf8(table->item(row, 1)->text())},
                {"parentId", table->item(row, 2)->text().toInt()},
                {"nexus", mappings},
            });
        }
        const auto* list = a_dialog->findChild<QListWidget*>("nexusCategoryList");
        Json nexus = Json::array();
        for (int row = 0; row < list->count(); ++row) {
            const auto* item = list->item(row);
            nexus.push_back({{"id", item->data(Qt::UserRole).toInt()}, {"name", Utf8(item->text())}});
        }
        return {{"categories", rows}, {"nexusCategories", nexus}, {"mappingEnabled", list->isEnabled()}};
    }

    void ContextAction(QTableWidget* a_table, int a_row, const char* a_action) {
        if (a_row >= 0) {
            a_table->scrollToItem(a_table->item(a_row, 0));
        }
        const QPoint point = a_row < 0 ? QPoint(0, -1) : a_table->visualItemRect(a_table->item(a_row, 0)).center();
        InvokeNativeMenu("QMenu", QCoreApplication::translate("CategoriesDialog", a_action), [a_table, point] {
            QMetaObject::invokeMethod(
                a_table,
                "customContextMenuRequested",
                Qt::DirectConnection,
                Q_ARG(QPoint, point)
            );
        });
    }

    void SetCell(QTableWidget* a_table, int a_row, int a_column, const Json& a_value) {
        const auto* item = a_table->item(a_row, a_column);
        const auto text = a_value.is_string() ? Text(a_value) : QString::number(a_value.get<int>());
        if (item->text() == text) {
            return;
        }
        const auto index = a_table->model()->index(a_row, a_column);
        const auto* delegate = a_table->itemDelegateForIndex(index);
        const std::unique_ptr<QWidget> editor(delegate->createEditor(a_table, QStyleOptionViewItem(), index));
        auto* line = qobject_cast<QLineEdit*>(editor.get());
        if (line == nullptr) {
            throw ToolError(409, "Category cell editor is unavailable");
        }
        line->setText(text);
        if (!line->hasAcceptableInput()) {
            throw ToolError(400, "Category value rejected by MO2's validator");
        }
        delegate->setModelData(editor.get(), a_table->model(), index);
    }

    void UpdateRow(QTableWidget* a_table, int a_row, const Json& a_values) {
        const std::map<std::string, int> columns {{"id", 0}, {"name", 1}, {"parentId", 2}};
        const auto* identity = RequireRow(a_table, a_row);
        for (const auto& [field, value] : a_values.items()) {
            SetCell(a_table, identity->row(), columns.at(field), value);
        }
    }

    void SetOrder(const QTableWidget* a_table, const Json& a_rows) {
        std::set<int> requested;
        for (const auto& row : a_rows) {
            RequireRow(a_table, row.get<int>());
            requested.insert(row.get<int>());
        }
        if (requested.size() != a_rows.size() || std::cmp_not_equal(requested.size(), a_table->rowCount())) {
            throw ToolError(400, "Order must contain every category row exactly once");
        }
        auto* header = a_table->verticalHeader();
        int position = 0;
        for (const auto& row : a_rows) {
            header->moveSection(header->visualIndex(row.get<int>()), position++);
        }
    }

    void SetMappings(const QWidget* a_dialog, QTableWidget* a_table, int a_row, const Json& a_ids) {
        const auto* list = a_dialog->findChild<QListWidget*>("nexusCategoryList");
        if (!list->isEnabled()) {
            throw ToolError(409, "Enable Nexus category mappings in Settings first");
        }
        QModelIndexList selection;
        for (const auto& identifier : a_ids) {
            bool found = false;
            for (int row = 0; row < list->count(); ++row) {
                if (list->item(row)->data(Qt::UserRole).toInt() == identifier.get<int>()) {
                    selection.push_back(list->model()->index(row, 0));
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw ToolError(
                    404,
                    std::format("Unknown Nexus category ID: {}. Read nexusCategories", identifier.get<int>())
                );
            }
        }
        const auto* identity = RequireRow(a_table, a_row);
        ContextAction(a_table, identity->row(), "Remove Nexus Mapping(s)");
        if (!selection.isEmpty()) {
            const std::unique_ptr<QMimeData> data(list->model()->mimeData(selection));
            if (!a_table->model()->dropMimeData(data.get(), Qt::CopyAction, identity->row(), 3, QModelIndex())) {
                throw ToolError(409, "MO2 rejected the Nexus category mapping");
            }
        }
    }
}

Json EditCategories(QWidget* a_dialog, const Json& a_arguments) {
    const auto action = a_arguments.at("action").get<std::string>();
    auto* table = a_dialog->findChild<QTableWidget*>("categoriesTable");
    if (action == "accept" || action == "cancel") {
        auto result = ReadCategories(a_dialog);
        auto* dialog = qobject_cast<QDialog*>(a_dialog);
        if (action == "accept") {
            dialog->accept();
        } else {
            dialog->reject();
        }
        result["accepted"] = action == "accept";
        return result;
    }
    if (action == "create") {
        std::set<int> before;
        for (int row = 0; row < table->rowCount(); ++row) {
            before.insert(table->item(row, 0)->text().toInt());
        }
        ContextAction(table, -1, "Add");
        for (int row = 0; row < table->rowCount(); ++row) {
            const auto identifier = table->item(row, 0)->text().toInt();
            if (!before.contains(identifier)) {
                UpdateRow(table, row, a_arguments.at("values"));
                break;
            }
        }
    } else if (action == "update") {
        UpdateRow(table, a_arguments.at("row").get<int>(), a_arguments.at("values"));
    } else if (action == "remove") {
        ContextAction(table, RequireRow(table, a_arguments.at("row").get<int>())->row(), "Remove");
    } else if (action == "setOrder") {
        SetOrder(table, a_arguments.at("rows"));
    } else if (action == "setMappings") {
        SetMappings(a_dialog, table, a_arguments.at("row").get<int>(), a_arguments.at("ids"));
    }
    auto result = ReadCategories(a_dialog);
    if (action == "list") {
        qobject_cast<QDialog*>(a_dialog)->reject();
    }
    return result;
}
}
