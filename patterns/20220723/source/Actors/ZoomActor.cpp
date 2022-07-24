#include "ZoomActor.hpp"
#include <QtWidgets/QGraphicsView>

void ZoomActor::zoomIn(QGraphicsView* view)
{
    auto r = zoomImpl(true);
    if (r.first && view) {
        view->scale(r.second, r.second);
    }
}

void ZoomActor::zoomOut(QGraphicsView* view)
{
    auto r = zoomImpl(false);
    if (r.first && view) {
        view->scale(r.second, r.second);
    }
}

std::pair<bool, double> ZoomActor::zoomImpl(bool bigOrSmall) noexcept
{
    auto zoom = m_config.zoom;
    auto zoomOutFactor = 1 / m_config.zoomInFactor;
    double zoomFactor = zoomOutFactor;
    if (bigOrSmall)
    {
        zoomFactor = m_config.zoomInFactor;
        zoom += m_config.zoomStep;
    }
    else
    {
        zoomFactor = zoomOutFactor;
        zoom -= m_config.zoomStep;
    }

    auto clamped = false;
    if (m_config.zoom < m_config.zoomRange.first)
    {
        zoom = m_config.zoomRange.first;
        clamped = true;
    }
    else if (m_config.zoom > m_config.zoomRange.second)
    {
        zoom = m_config.zoomRange.second;
        clamped = true;
    }

    return std::make_pair(m_config.zoomClamp == false || clamped == false,
        zoomFactor);
}
