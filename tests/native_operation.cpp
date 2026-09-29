#include "EventBus.h"
#include "Harness.h"
#include "Native/DialogSnapshot.h"
#include "Native/Operation.h"
#include <QApplication>
#include <QDialog>
#include <QTimer>
#include <stdexcept>

int main(int a_count, char** a_args) {
    return Tests::Run([a_count, a_args] {
        int count = a_count;
        QApplication app(count, a_args);
        Bench::EventBus events;
        Bench::NativeOperation operation(events, &app);
        QDialog dialog;
        bool awaiting = false;

        const auto started = operation.Start("edit", [&] {
            QTimer::singleShot(0, &dialog, [&] {
                awaiting = operation.Status()["state"] == "awaitingNativeDialog";
                dialog.accept();
            });
            return Bench::Json {{"accepted", dialog.exec() == QDialog::Accepted}};
        });
        if (started["state"] != "scheduled") {
            return 1;
        }
        QApplication::processEvents();
        QApplication::processEvents();
        const auto finished = operation.Status();
        if (!awaiting || finished["state"] != "finished" || finished["result"]["accepted"] != true) {
            return 2;
        }

        operation.Start("edit", [] -> Bench::Json { throw std::runtime_error("Native edit failed"); });
        QApplication::processEvents();
        const auto failed = operation.Status();
        if (failed["state"] != "failed" || failed["error"] != "Native edit failed") {
            return 3;
        }
        dialog.setWindowTitle("Snapshot editor");
        const auto snapshot = Bench::SnapshotNativeDialog(
            "QDialog",
            [&] { dialog.exec(); },
            [](QDialog* a_editor) { return Bench::Json {{"title", Bench::Utf8(a_editor->windowTitle())}}; }
        );
        if (snapshot["title"] != "Snapshot editor" || dialog.isVisible()) {
            return 4;
        }
        bool caught = false;
        try {
            Bench::SnapshotNativeDialog(
                "QDialog",
                [&] { dialog.exec(); },
                [](QDialog*) -> Bench::Json { throw std::runtime_error("Cannot read editor"); }
            );
        } catch (const std::runtime_error&) {
            caught = true;
        }
        if (!caught || dialog.isVisible()) {
            return 5;
        }
        return 0;
    });
}
