#pragma once
#include <utility>
#include <any>
#include <QtWidgets/QGraphicsView>
#include <QtGui/QMouseEvent>

class GraphicsViewImpl :public QGraphicsView
{
public:
    using QGraphicsView::QGraphicsView;

    void mousePressEvent(QMouseEvent* event) override {
        return QGraphicsView::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        return QGraphicsView::mouseReleaseEvent(event);
    }
};

/// @brief 缩放设置
struct ZoomSetting
{
    double zoomInFactor = 1.25;
    bool zoomClamp = true;
    double zoom = 10.0;
    double zoomStep = 1.0;
    std::pair<double, double> zoomRange = { 0, 20 };

    /// @brief 缩放
    /// @param bigOrSmall 
    /// @return 
    std::pair<bool,double> zoomImpl(bool bigOrSmall) noexcept
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

        return std::make_pair(zoomClamp == false || clamped == false,
            zoomFactor);
    }
};

/// @brief 缩放动作
struct ZoomAction
{
    bool bigOrSmall = true;
};

/// @brief 缩放实现动作
struct ZoomImplAction
{
    double factor;
};

/// @brief 缩放处理
/// @param cfg 
/// @param action 
void ZoomHandler(Dispatcher& dispatcher,ZoomSetting& cfg, ZoomAction action)
{
    auto [result,factor] = cfg.zoomImpl(action.bigOrSmall);
    if (result) {
        dispatcher.post(ZoomImplAction{factor});
    }
}

/// @brief 视图缩放实现
/// @param view 
/// @param action 
void ZoomImplHandler(QGraphicsView* view, ZoomImplAction action) {
    if (view) {
        view->scale(action.factor, action.factor);
    }
}

struct PanBeginAction {
    GraphicsViewImpl* target = nullptr;
    QMouseEvent* event;
};

struct PanEndAction {
    GraphicsViewImpl* target = nullptr;
    QMouseEvent* event;
};

void PanBeginHandler(PanBeginAction action)
{
    auto [view, event] = action;
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event->localPos(), event->screenPos(),
        Qt::LeftButton, Qt::NoButton, event->modifiers());
    view->mouseReleaseEvent(&releaseEvent);

    //切换到滚动模式来完成平移动作
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    QMouseEvent fakeEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
        event->buttons() | Qt::LeftButton, event->modifiers());
    view->mousePressEvent(&fakeEvent);
}

void PanEndHandler(PanEndAction action)
{
    auto [view, event] = action;
    //鼠标中键释放后切换回原有模式
    QMouseEvent fakeEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
        event->buttons() & ~Qt::LeftButton, event->modifiers());
    view->mouseReleaseEvent(&fakeEvent);
    view->setDragMode(QGraphicsView::RubberBandDrag);
}
