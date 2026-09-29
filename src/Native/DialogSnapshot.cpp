#include "Native/DialogSnapshot.h"
#include "Json.h"
#include "Tools/Registry.h"
#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <exception>
#include <functional>
#include <utility>

namespace Bench {
namespace {
    class DialogSnapshot : public QObject {
    public:
        DialogSnapshot(QString a_className, std::function<Json(QDialog*)> a_read)
            : _className(std::move(a_className))
            , _read(std::move(a_read)) {
            qApp->installEventFilter(this);
        }

        DialogSnapshot(const DialogSnapshot&) = delete;
        DialogSnapshot& operator=(const DialogSnapshot&) = delete;
        DialogSnapshot(DialogSnapshot&&) = delete;
        DialogSnapshot& operator=(DialogSnapshot&&) = delete;

        ~DialogSnapshot() override {
            qApp->removeEventFilter(this);
        }

        [[nodiscard]] Json Result() const {
            if (_error) {
                std::rethrow_exception(_error);
            }
            if (_result.is_null()) {
                throw ToolError(409, "Native editor did not return a snapshot");
            }
            return _result;
        }

    protected:
        bool eventFilter(QObject* a_object, QEvent* a_event) override {
            if (a_event->type() != QEvent::Show || QString(a_object->metaObject()->className()) != _className) {
                return false;
            }
            qApp->removeEventFilter(this);
            const QPointer<QDialog> dialog = qobject_cast<QDialog*>(a_object);
            QTimer::singleShot(0, this, [this, dialog] {
                if (!dialog) {
                    return;
                }
                try {
                    _result = _read(dialog);
                } catch (...) {
                    _error = std::current_exception();
                }
                dialog->reject();
            });
            return false;
        }

    private:
        QString _className;
        std::function<Json(QDialog*)> _read;
        Json _result;
        std::exception_ptr _error;
    };
}

Json SnapshotNativeDialog(
    const QString& a_className,
    const std::function<void()>& a_open,
    std::function<Json(QDialog*)> a_read
) {
    if (QApplication::activeModalWidget() != nullptr) {
        throw ToolError(409, "Close the current modal dialog before taking a snapshot");
    }
    const DialogSnapshot snapshot(a_className, std::move(a_read));
    a_open();
    return snapshot.Result();
}
}
