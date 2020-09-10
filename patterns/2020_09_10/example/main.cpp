#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include "dispatcher.hpp"
#include "store.hpp"
#include "view.hpp"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    //QLabel view{u8"unidirectional data flow"};
    //view.show();

    Dispatcher dispatcher{nullptr};
    Store store{ &dispatcher };
    CounterView view{nullptr};
    view.bind(&dispatcher, &store);

    //首先有action触发调整store
    QObject::connect(&dispatcher, &Dispatcher::dispatched, &store, &Store::onDispatched);
    //然后手动刷新视图
    QObject::connect(&dispatcher, &Dispatcher::dispatched,
        [&](QString action, QMap<QString, QVariant> args) {
            view.refresh();
        });

    view.show();
    return app.exec();
}
