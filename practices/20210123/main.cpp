#include <stdexcept>
#include <QtWidgets/QApplication>
#include <QtGui/QMouseEvent>
#include "Dispatcher.hpp"
#include "GraphicsViewHandler.hpp"


class GraphicsViewEventFilter :public QObject
{
    GraphicsViewImpl* m_target = nullptr;
    Dispatcher* m_dispatcher = nullptr;
public:
    GraphicsViewEventFilter(GraphicsViewImpl* target,
        Dispatcher* dispatcher,
        QObject* parent = nullptr)
        :QObject(parent),m_target(target),m_dispatcher(dispatcher) {
        if (m_target == nullptr || m_dispatcher == nullptr)
            throw std::invalid_argument("invalid ctor argument");
    };
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched != m_target && watched != m_target->viewport()) {
            return QObject::eventFilter(watched, event);
        }
        if (event->type() == QEvent::Wheel) {
            //在滚动模式下不缩放,所以不拦截事件
            if (m_target->dragMode() == QGraphicsView::ScrollHandDrag)
                return false;
            auto e = static_cast<QWheelEvent*>(event);
            m_dispatcher->dispatch(ZoomAction{ e->angleDelta().y() > 0});
            return true;
        }
        if (event->type() == QEvent::KeyPress)
        {
            auto e = static_cast<QKeyEvent*>(event);
            if (e->matches(QKeySequence::ZoomIn)) {
                m_dispatcher->dispatch(ZoomAction{ true });
                return true;
            }
            else if (e->matches(QKeySequence::ZoomOut)) {
                m_dispatcher->dispatch(ZoomAction{ false });
                return true;
            }
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            auto e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::MiddleButton) {
                m_dispatcher->dispatch(PanBeginAction{m_target,e});
                return true;
            }
            return false;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::MiddleButton) {
                m_dispatcher->dispatch(PanEndAction{ m_target,e });
                return true;
            }
            return false;
        }
        return false;
    }
};


int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    Dispatcher   dispatcher;
    ZoomSetting  zoomSetting;
    dispatcher.registerHandler("GraphicsViewZoomHandler",
        [&](std::any&& v) {
            if (ZoomAction* action = std::any_cast<ZoomAction>(&v); action) {
                ZoomHandler(dispatcher, zoomSetting, *action);
            }
        });

    GraphicsViewImpl appView;
    GraphicsViewEventFilter filter{ &appView,&dispatcher,&app };
    //注册事件过滤器,注册action处理
    appView.installEventFilter(&filter);
    appView.viewport()->installEventFilter(&filter);

    dispatcher.registerHandler("GraphicsViewZoomImplHandler",
        [&](std::any&& v) {
            if (ZoomImplAction* action = std::any_cast<ZoomImplAction>(&v); action) {
                ZoomImplHandler(&appView, *action);
            }
        });
    dispatcher.registerHandler("GraphicsViewPanBeginHandler",
        [&](std::any&& v) {
            if (PanBeginAction* action = std::any_cast<PanBeginAction>(&v); action) {
                PanBeginHandler(*action);
            }
        });
    dispatcher.registerHandler("GraphicsViewPanEndHandler",
        [&](std::any&& v) {
            if (PanEndAction* action = std::any_cast<PanEndAction>(&v); action) {
                PanEndHandler(*action);
            }
        });
    {//初始化
        double w = 64000.0;
        double h = 32000.0;

        appView.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        appView.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        appView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        appView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        appView.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        appView.setDragMode(QGraphicsView::RubberBandDrag);

        appView.setScene(new QGraphicsScene(&appView));
        appView.setSceneRect(-w / 2, -h / 2, w, h);
    }
    {//添加几个graphicsItem
        appView.scene()->addSimpleText(QString(u8"liff.engineer@gmail.com"));
        appView.scene()->addRect(-200.0, -150.0, 400.0, 300.0);
    }
    appView.show();
    return app.exec();
}
