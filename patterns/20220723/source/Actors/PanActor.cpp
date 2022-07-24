#include "PanActor.hpp"
#include "View/GraphicsView.hpp"
#include <QtGui/QMouseEvent>

void PanActor::BeginPan(QGraphicsView* source, QMouseEvent* event)
{
    auto view = dynamic_cast<GraphicsView*>(source);
    if (!view || !event) return;

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event->localPos(), event->screenPos(),
        Qt::LeftButton, Qt::NoButton, event->modifiers());
    view->mouseReleaseEvent(&releaseEvent);

    //切换到滚动模式来完成平移动作
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    QMouseEvent fakeEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
        event->buttons() | Qt::LeftButton, event->modifiers());
    view->mousePressEvent(&fakeEvent);
}

void PanActor::EndPan(QGraphicsView* source, QMouseEvent* event)
{
    auto view = dynamic_cast<GraphicsView*>(source);
    if (!view || !event) return;

    //鼠标中键释放后切换回原有模式
    QMouseEvent fakeEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
        event->buttons() & ~Qt::LeftButton, event->modifiers());
    view->mouseReleaseEvent(&fakeEvent);
    view->setDragMode(QGraphicsView::RubberBandDrag);
}
