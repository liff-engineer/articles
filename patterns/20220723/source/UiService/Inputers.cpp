#include "Inputers.hpp"
#include <QtWidgets/QGraphicsView>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QMessageBox>
#include <stdexcept>

namespace
{
    class GraphicsViewEventFilter :public QObject
    {
        QGraphicsView* m_target{ nullptr };
        InputSource* m_input{ nullptr };
    public:
        GraphicsViewEventFilter(QGraphicsView* target, InputSource* input, QObject* parent = nullptr)
            :QObject(parent), m_target(target), m_input(input) {
            if (m_target == nullptr || m_input == nullptr)
                throw std::invalid_argument("invalid ctor argument");
            target->installEventFilter(this);
            target->viewport()->installEventFilter(this);
        };

        ~GraphicsViewEventFilter() noexcept {
            if (m_target) {
                m_target->removeEventFilter(this);
                m_target->viewport()->removeEventFilter(this);
            }
        }

        bool eventFilter(QObject* watched, QEvent* event) override {
            if (watched != m_target && watched != m_target->viewport()) {
                return QObject::eventFilter(watched, event);
            }
            return m_input->eventFilter(event);
        }
    };
}


InputSource::InputSource(QGraphicsView* view)
    :m_source(view)
{
    m_eventFilter = std::make_unique<GraphicsViewEventFilter>(view, this);
}

void InputSource::uninstallInputer(std::size_t index)
{
    if (index < m_inputers.size()) {
        m_inputers[index] = nullptr;
    }
}

bool InputSource::eventFilter(QEvent* event)
{
    for (auto& obj : m_inputers) {
        if (obj) {
            if (obj(m_source, event)) {
                return true;
            }
        }
    }
    return false;
}

ViewControlInputer::ViewControlInputer(InputSource* source)
    :m_source(source)
{
    m_stub = m_source->installInputer([&](QGraphicsView* target, QEvent* event)->bool {
        return this->eventFilter(target, event);
        });
}

void ViewControlInputer::stop()
{
    if (m_stub) {
        m_source->uninstallInputer(static_cast<std::size_t>(m_stub));
    }
}

bool ViewControlInputer::eventFilter(QGraphicsView* target, QEvent* event)
{
    if (event->type() == QEvent::Wheel) {
        //在滚动模式下不缩放,所以不拦截事件
        if (target->dragMode() == QGraphicsView::ScrollHandDrag)
            return false;
        auto e = static_cast<QWheelEvent*>(event);
        if (e->angleDelta().y() > 0) {
            m_zoomActor.zoomIn(target);
        }
        else
        {
            m_zoomActor.zoomOut(target);
        }
        return true;
    }
    if (event->type() == QEvent::KeyPress)
    {
        auto e = static_cast<QKeyEvent*>(event);
        if (e->matches(QKeySequence::ZoomIn)) {
            m_zoomActor.zoomIn(target);
            return true;
        }
        else if (e->matches(QKeySequence::ZoomOut)) {
            m_zoomActor.zoomOut(target);
            return true;
        }
        return false;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::MiddleButton) {

            m_panActor.BeginPan(target, e);
            return true;
        }
        return false;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        auto e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::MiddleButton) {
            m_panActor.EndPan(target, e);
            return true;
        }
        return false;
    }
    return false;
}

FlowControlSignalInputer::FlowControlSignalInputer(InputSource* source, std::function<void()> exitNofityer)
    :m_exitNofityer(exitNofityer)
{
    source->installInputer([&](QGraphicsView* target, QEvent* event)->bool {
        return this->eventFilter(target, event);
        });
}

bool FlowControlSignalInputer::eventFilter(QGraphicsView* target, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto e = static_cast<QKeyEvent*>(event);
        if (e->matches(QKeySequence::Cancel)) {
            if (m_exitNofityer) {
                if (QMessageBox::question(target, "Test", "Quit?",
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    m_exitNofityer();
                    return true;
                }
            }
        }
        return false;
    }
    return false;
}

PointInputer::PointInputer(InputSource* source)
{
    source->installInputer([&](QGraphicsView* target, QEvent* event)->bool {
        return this->eventFilter(target, event);
        });
}

void PointInputer::stop()
{
    if (m_item) {
        m_item->scene()->removeItem(m_item);
        delete m_item;
        m_item = nullptr;
    }
}

bool PointInputer::eventFilter(QGraphicsView* target, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        auto e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton)
        {
            //左键点击则创建
            auto item = target->scene()->addEllipse(m_item->rect(), m_item->pen(), m_item->brush());
            item->setPos(m_item->pos());
            return true;
        }
        return false;
    }
    else if (event->type() == QEvent::MouseMove)
    {
        auto e = static_cast<QMouseEvent*>(event);
        auto pos = target->mapToScene(e->pos());
        {//鼠标移动则更新item
            if (!m_item) {
                m_item = new QGraphicsEllipseItem(10.0,10.0,10.0,10.0);
                m_item->setBrush(Qt::red);
                target->scene()->addItem(m_item);
            }
            m_item->setPos(QPointF{ pos.x(),pos.y() });
        }
        return false;
    }
    return false;
}
