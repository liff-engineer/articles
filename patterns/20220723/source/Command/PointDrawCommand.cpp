#include "PointDrawCommand.hpp"
#include "UiService/UiContext.hpp"
#include "UiService/Inputers.hpp"
#include <QtWidgets/QGraphicsView>

void PointDrawCommand::start(UiContext* ctx, std::function<void(ICommand*)> stopNotifyer)
{
    if (!ctx) return;
    auto view = ctx->find<QGraphicsView>();
    if (!view) return;

    m_stopNotifyer = stopNotifyer;
    m_input = std::make_unique<UiService>();

    auto source = m_input->install<InputSource>(view);
    m_input->install<ViewControlInputer>(source);
    m_input->install<FlowControlSignalInputer>(source, 
        [&]() {
            if (m_stopNotifyer) {
                m_stopNotifyer(this);
            }
        });
    m_input->install<PointInputer>(source);
}

void PointDrawCommand::stop()
{
    m_input->stop();
    m_input = nullptr;
    m_stopNotifyer = nullptr;
}
