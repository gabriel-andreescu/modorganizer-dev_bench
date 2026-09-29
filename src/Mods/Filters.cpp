#include "Mods/Filters.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMainWindow>
#include <QObject>
#include <QPushButton>
#include <QRadioButton>
#include <QTreeWidget>
#include <QVariant>
#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <iterator>
#include <qnamespace.h>
#include <string>

namespace Bench {
namespace {
    constexpr int kStateRole = Qt::UserRole + 2;
    constexpr std::array<const char*, 3> kStates {"inactive", "include", "exclude"};

    QWidget* MainWindow() {
        if (QApplication::activeModalWidget() != nullptr) {
            throw ToolError(409, "Answer the current modal dialog first");
        }
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* window = qobject_cast<QMainWindow*>(widget)) {
                return window;
            }
        }
        throw ToolError(409, "MO2 main window is unavailable");
    }

    Json ComboState(const QComboBox* a_combo) {
        Json choices = Json::array();
        for (int i = 0; i < a_combo->count(); ++i) {
            choices.push_back({{"index", i}, {"text", Utf8(a_combo->itemText(i))}});
        }
        return {{"index", a_combo->currentIndex()}, {"choices", choices}, {"enabled", a_combo->isEnabled()}};
    }

    Json ReadFilters(const QWidget* a_window) {
        const auto* tree = a_window->findChild<QTreeWidget*>("filters");
        Json criteria = Json::array();
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            const auto* item = tree->topLevelItem(i);
            criteria.push_back({
                {"index", i},
                {"text", Utf8(item->text(1))},
                {"id", item->data(0, Qt::UserRole).toInt()},
                {"type", item->data(0, Qt::UserRole + 1).toInt()},
                {"state", kStates.at(item->data(0, kStateRole).toInt())},
            });
        }
        return {
            {"text", Utf8(a_window->findChild<QLineEdit*>("modFilterEdit")->text())},
            {"mode", a_window->findChild<QRadioButton*>("filtersAnd")->isChecked() ? "all" : "any"},
            {"criteria", criteria},
            {"grouping", ComboState(a_window->findChild<QComboBox*>("groupCombo"))},
            {"separators", ComboState(a_window->findChild<QComboBox*>("filtersSeparators"))},
        };
    }

    void SetCriterion(QTreeWidget* a_tree, const Json& a_entry) {
        const auto index = a_entry.at("index").get<int>();
        if (index < 0 || index >= a_tree->topLevelItemCount()) {
            throw ToolError(400, "Filter criterion index is out of range");
        }
        auto* item = a_tree->topLevelItem(index);
        const auto state = a_entry.at("state").get<std::string>();
        const auto target = std::ranges::find(kStates, state);
        if (target == kStates.end()) {
            throw ToolError(400, "Unknown filter state");
        }
        const auto steps = (std::distance(kStates.begin(), target) - item->data(0, kStateRole).toInt() + 3) % 3;
        a_tree->setCurrentItem(item);
        for (int i = 0; i < steps; ++i) {
            QKeyEvent event(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
            QApplication::sendEvent(a_tree, &event);
        }
    }

    void SetComboIndex(QComboBox* a_combo, int a_index) {
        if (!a_combo->isEnabled()) {
            throw ToolError(409, "MO2 disabled this filter option");
        }
        if (a_index < 0 || a_index >= a_combo->count()) {
            throw ToolError(400, "Filter option index is out of range");
        }
        a_combo->setCurrentIndex(a_index);
    }

    void UpdateFilters(const QWidget* a_window, const Json& a_filter) {
        if (a_filter.contains("grouping")) {
            SetComboIndex(a_window->findChild<QComboBox*>("groupCombo"), a_filter.at("grouping").get<int>());
        }
        if (a_filter.contains("text")) {
            a_window->findChild<QLineEdit*>("modFilterEdit")->setText(Text(a_filter.at("text")));
        }
        if (a_filter.contains("mode")) {
            const auto* name = a_filter.at("mode") == "all" ? "filtersAnd" : "filtersOr";
            a_window->findChild<QRadioButton*>(name)->click();
        }
        if (a_filter.contains("separators")) {
            SetComboIndex(a_window->findChild<QComboBox*>("filtersSeparators"), a_filter.at("separators").get<int>());
        }
        if (a_filter.contains("criteria")) {
            for (const auto& entry : a_filter.at("criteria")) {
                SetCriterion(a_window->findChild<QTreeWidget*>("filters"), entry);
            }
        }
    }
}

Json WithUnfilteredModList(const std::function<Json()>& a_operation) {
    const auto* window = MainWindow();
    const auto state = ReadFilters(window);
    const Json restore = {{"text", state.at("text")}, {"criteria", state.at("criteria")}};
    window->findChild<QPushButton*>("clearFiltersButton")->click();

    Json result;
    std::exception_ptr error;
    try {
        result = a_operation();
    } catch (...) {
        error = std::current_exception();
    }
    UpdateFilters(window, restore);
    if (error) {
        std::rethrow_exception(error);
    }
    return result;
}

Json ModFilters(const Json& a_arguments) {
    const auto* window = MainWindow();
    if (a_arguments.at("action") == "setFilters") {
        UpdateFilters(window, a_arguments.at("filter"));
    } else if (a_arguments.at("action") == "clearFilters") {
        window->findChild<QPushButton*>("clearFiltersButton")->click();
    }
    return ReadFilters(window);
}
}
