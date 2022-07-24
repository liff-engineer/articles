#include <QtWidgets/QApplication>
#include "View/ApplicationView.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    ApplicationView appView{};
    appView.show();
    return app.exec();
}
