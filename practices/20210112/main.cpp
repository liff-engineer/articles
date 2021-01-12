#include <QtWidgets/QApplication>
#include "GraphicsView.hpp"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    GraphicsView appView{nullptr};
    {
        appView.scene()->addSimpleText(QString(u8"liff.engineer@gmail.com"));
        appView.scene()->addRect(-200.0, -150.0, 400.0, 300.0);
    }
    appView.show();
    return app.exec();
}