#include "GraphicsView.hpp"
#include <QtGui/QMouseEvent>

GraphicsView::GraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::RubberBandDrag);

    setScene(new QGraphicsScene(this));
    setSceneRect(-w / 2, -h / 2, w, h);
}

void GraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::ZoomIn))
    {
        zoomImpl(true);
    }
    else if (event->matches(QKeySequence::ZoomOut))
    {
        zoomImpl(false);
    }
    else
    {
        QGraphicsView::keyPressEvent(event);
    }
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    if (dragMode() == QGraphicsView::ScrollHandDrag)
        return;
    return zoomImpl(event->angleDelta().y() > 0);
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        return panBeginImpl(event);
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        return panEndImpl(event);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::zoomImpl(bool bigOrSmall)
{
    auto zoomOutFactor = 1 / zoomInFactor;
    double zoomFactor = zoomOutFactor;
    if (bigOrSmall)
    {
        zoomFactor = zoomInFactor;
        zoom += zoomStep;
    }
    else
    {
        zoomFactor = zoomOutFactor;
        zoom -= zoomStep;
    }

    auto clamped = false;
    if (zoom < zoomRange.first)
    {
        zoom = zoomRange.first;
        clamped = true;
    }
    else if (zoom > zoomRange.second)
    {
        zoom = zoomRange.second;
        clamped = true;
    }

    if (zoomClamp == false || clamped == false)
    {
        this->scale(zoomFactor, zoomFactor);
    }
}

void GraphicsView::panBeginImpl(QMouseEvent *event)
{
    auto releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, event->localPos(), event->screenPos(),
                                        Qt::LeftButton, Qt::NoButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(releaseEvent);
    //切换到滚动模式来完成平移动作
    setDragMode(QGraphicsView::ScrollHandDrag);
    auto fakeEvent = new QMouseEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
                                     event->buttons() | Qt::LeftButton, event->modifiers());
    QGraphicsView::mousePressEvent(fakeEvent);
}
void GraphicsView::panEndImpl(QMouseEvent *event)
{
    //鼠标中键释放后切换回原有模式
    auto fakeEvent = new QMouseEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
                                     event->buttons() & ~Qt::LeftButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(fakeEvent);
    setDragMode(QGraphicsView::RubberBandDrag);
}