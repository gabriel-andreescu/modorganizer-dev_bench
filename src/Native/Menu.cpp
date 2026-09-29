#include "Native/Menu.h"
#include "Tools/Registry.h"
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <exception>
#include <functional>
#include <utility>

namespace Bench {
namespace {
    class MenuAction : public QObject {
    public:
        MenuAction(QString a_className, QString a_action)
            : _className(std::move(a_className))
            , _label(std::move(a_action)) {
            qApp->installEventFilter(this);
        }

        MenuAction(const MenuAction&) = delete;
        MenuAction& operator=(const MenuAction&) = delete;
        MenuAction(MenuAction&&) = delete;
        MenuAction& operator=(MenuAction&&) = delete;

        ~MenuAction() override {
            qApp->removeEventFilter(this);
        }

        void Check() const {
            if (_error) {
                std::rethrow_exception(_error);
            }
            if (!_invoked) {
                throw ToolError(409, "Native menu action is unavailable");
            }
        }

    protected:
        bool eventFilter(QObject* a_object, QEvent* a_event) override {
            if (a_event->type() != QEvent::Show || QString(a_object->metaObject()->className()) != _className) {
                return false;
            }
            qApp->removeEventFilter(this);
            const QPointer<QMenu> menu = qobject_cast<QMenu*>(a_object);
            QTimer::singleShot(0, this, [this, menu] {
                if (menu) {
                    Trigger(menu);
                }
            });
            return false;
        }

    private:
        void Trigger(QMenu* a_menu) {
            a_menu->close();
            for (auto* action : a_menu->actions()) {
                if (action->text() == _label && action->isEnabled()) {
                    try {
                        action->trigger();
                        _invoked = true;
                    } catch (...) {
                        _error = std::current_exception();
                    }
                    return;
                }
            }
        }

        QString _className;
        QString _label;
        bool _invoked = false;
        std::exception_ptr _error;
    };

}

void InvokeNativeMenu(const QString& a_className, const QString& a_action, const std::function<void()>& a_open) {
    const MenuAction operation(a_className, a_action);
    a_open();
    operation.Check();
}
}
