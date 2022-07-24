#include "ViewCommand.hpp"
#include <stdexcept>
#include <QtGui/QWheelEvent>

#include "View/GraphicsView.hpp"
#include "UiService/UiContext.hpp"

#include "Actors/PanActor.hpp"
#include "Actors/ZoomActor.hpp"


namespace
{
    class GraphicsViewEventFilter :public QObject
    {
        GraphicsView* m_target{ nullptr };
        PanActor m_panActor{};
        ZoomActor m_zoomActor{};
    public:
        GraphicsViewEventFilter(GraphicsView* target, QObject* parent = nullptr)
            :QObject(parent), m_target(target) {
            if (m_target == nullptr)
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
            if (event->type() == QEvent::Wheel) {
                //在滚动模式下不缩放,所以不拦截事件
                if (m_target->dragMode() == QGraphicsView::ScrollHandDrag)
                    return false;
                auto e = static_cast<QWheelEvent*>(event);
                if (e->angleDelta().y() > 0) {
                    m_zoomActor.zoomIn(m_target);
                }
                else
                {
                    m_zoomActor.zoomOut(m_target);
                }
                return true;
            }
            if (event->type() == QEvent::KeyPress)
            {
                auto e = static_cast<QKeyEvent*>(event);
                if (e->matches(QKeySequence::ZoomIn)) {
                    m_zoomActor.zoomIn(m_target);
                    return true;
                }
                else if (e->matches(QKeySequence::ZoomOut)) {
                    m_zoomActor.zoomOut(m_target);
                    return true;
                }
                return false;
            }
            if (event->type() == QEvent::MouseButtonPress) {
                auto e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::MiddleButton) {
                    
                    m_panActor.BeginPan(m_target,e);
                    return true;
                }
                return false;
            }
            if (event->type() == QEvent::MouseButtonRelease) {
                auto e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::MiddleButton) {
                    m_panActor.EndPan(m_target, e);
                    return true;
                }
                return false;
            }
            return false;
        }
    };
}

void ViewCommand::start(UiContext* ctx, std::function<void(ICommand*)> stopNotifyer)
{
    if (!ctx) return;
    auto view = ctx->find<GraphicsView>();
    if (!view) return;

    auto inputer = std::make_unique<GraphicsViewEventFilter>(view);
    view->installEventFilter(inputer.get());
    view->viewport()->installEventFilter(inputer.get());
    m_inputer = std::move(inputer);
    m_stopNotifyer = stopNotifyer;
}

void ViewCommand::stop()
{
    m_inputer = nullptr;
    m_stopNotifyer = nullptr;
}
