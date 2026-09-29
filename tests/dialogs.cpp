#include "Dialogs/Actions.h"
#include "Harness.h"
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <cstdint>

namespace {
const Bench::Json* Control(const Bench::Json& a_state, const std::string& a_text) {
    for (const auto& control : a_state["controls"]) {
        if (control.value("text", "") == a_text) {
            return &control;
        }
    }
    return nullptr;
}

int CheckChoice(Bench::DialogActions& a_actions, QCheckBox& a_choice) {
    const auto state = a_actions.Describe();
    const auto* control = Control(state, "High resolution");
    if (control == nullptr) {
        return 2;
    }
    if ((*control)["description"] != "Installs high resolution textures.") {
        return 1;
    }
    const std::uint64_t identifier = (*control)["id"];
    int clicks = 0;
    QObject::connect(&a_choice, &QCheckBox::clicked, [&clicks] { ++clicks; });
    a_actions.Invoke({{"action", "setChecked"}, {"id", identifier}, {"checked", true}});
    if (!a_choice.isChecked() || clicks != 1) {
        return 3;
    }
    a_choice.hide();
    try {
        a_actions.Invoke({{"action", "click"}, {"id", identifier}});
    } catch (const std::exception&) {
        return 0;
    }
    return 4;
}

int CheckReveal(Bench::DialogActions& a_actions, QVBoxLayout& a_layout) {
    QScrollArea area;
    auto* contents = new QWidget;
    contents->resize(300, 1800);
    auto* distant = new QCheckBox("Distant option", contents);
    distant->move(20, 1600);
    area.setWidget(contents);
    area.setFixedSize(340, 200);
    a_layout.addWidget(&area);
    QApplication::processEvents();
    const auto state = a_actions.Describe();
    const auto* control = Control(state, "Distant option");
    if (control == nullptr || (*control)["inViewport"] != false) {
        return 5;
    }
    const std::uint64_t identifier = (*control)["id"];
    a_actions.Invoke({{"action", "reveal"}, {"id", identifier}});
    if (area.verticalScrollBar()->value() == 0) {
        return 6;
    }
    a_actions.Invoke({{"action", "setChecked"}, {"id", identifier}, {"checked", true}});
    const auto choices = a_actions.Invoke({{"action", "choices"}, {"filter", "Distant"}});
    if (choices["groups"].size() != 1 || choices["groups"][0]["options"][0]["checked"] != true) {
        return 7;
    }
    return 0;
}

int CheckFileDialog(Bench::DialogActions& a_actions) {
    const QTemporaryDir directory;
    const auto path = directory.filePath("fixture.txt");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return 8;
    }
    file.close();

    QFileDialog picker;
    picker.setOption(QFileDialog::DontUseNativeDialog);
    picker.setFileMode(QFileDialog::ExistingFile);
    picker.setDirectory(directory.path());
    picker.setModal(true);
    picker.show();
    QApplication::processEvents();
    const auto fileId = a_actions.Describe()["id"];
    const auto selected = a_actions.Invoke({{"action", "selectFile"}, {"id", fileId}, {"path", path.toStdString()}});
    if (selected["fileSelection"]["selectedFiles"] != Bench::Json::array({path.toStdString()})) {
        return 9;
    }
    a_actions.Invoke({{"action", "accept"}, {"id", fileId}});
    QApplication::processEvents();
    if (picker.result() != QDialog::Accepted || picker.isVisible()) {
        return 10;
    }
    return 0;
}
}

int main(int a_count, char** a_args) {
    return Tests::Run([a_count, a_args] {
        int count = a_count;
        const QApplication app(count, a_args);
        QDialog dialog;
        QVBoxLayout layout(&dialog);
        QLabel description("Select optional textures");
        QCheckBox choice("High resolution");
        choice.setProperty("description", "Installs high resolution textures.");
        layout.addWidget(&description);
        layout.addWidget(&choice);
        dialog.show();
        QApplication::processEvents();
        Bench::DialogActions actions;
        return Tests::First({
            [&] { return CheckChoice(actions, choice); },
            [&] { return CheckReveal(actions, layout); },
            [&] {
                dialog.hide();
                return CheckFileDialog(actions);
            },
        });
    });
}
