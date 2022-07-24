#include "UiService/UiService.hpp"

#include <QObject>
#include <memory>
#include <functional>
#include <QtWidgets/QGraphicsItem>

#include "Actors/PanActor.hpp"
#include "Actors/ZoomActor.hpp"

class QGraphicsView;

/// @brief TODO FIXME:提取成接口,并注册到UiContext上
class InputSource final :public IInputer {
public:
    explicit InputSource(QGraphicsView* view);

    template<typename Fn>
    std::size_t installInputer(Fn&& fn) {
        m_inputers.emplace_back(std::forward<Fn>(fn));
        return m_inputers.size() - 1;
    }

    void uninstallInputer(std::size_t index);

    bool eventFilter(QEvent* event);
private:
    QGraphicsView* m_source;
    std::unique_ptr<QObject> m_eventFilter;
    std::vector<std::function<bool(QGraphicsView*, QEvent*)>> m_inputers;
};

/// @brief 视图控制输入,直接处理了,不再向外发送相关消息
class ViewControlInputer :public IInputer {
public:
    explicit ViewControlInputer(InputSource* source);
    void stop() final;
private:
    bool eventFilter(QGraphicsView* target,QEvent* event);
private:
    InputSource* m_source{};
    int m_stub{-1};
    PanActor m_panActor{};
    ZoomActor m_zoomActor{};
};

/// @brief 流程控制信号输入,当用户按下ESC时退出
class FlowControlSignalInputer : public IInputer {
public:
    explicit FlowControlSignalInputer(InputSource* source,std::function<void()> exitNofityer);
private:
    bool eventFilter(QGraphicsView* target, QEvent* event);
private:
    std::function<void()> m_exitNofityer{};
};

/// @brief 点输入,暂时先用画圆示意
class PointInputer :public IInputer {
public:
    explicit PointInputer(InputSource* source);
    void stop() final;
private:
    bool eventFilter(QGraphicsView* target, QEvent* event);
private:
    QGraphicsEllipseItem* m_item{};
};
