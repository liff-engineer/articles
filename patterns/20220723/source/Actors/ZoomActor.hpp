#pragma once
#include <utility>

class QGraphicsView;
class ZoomActor {
public:
    struct Config {
        double zoomInFactor{ 1.25 };
        bool zoomClamp{ true };
        double zoom{ 10.0 };
        double zoomStep{ 1.0 };
        std::pair<double, double> zoomRange{ 0, 20 };
    };

    void zoomIn(QGraphicsView* view);
    void zoomOut(QGraphicsView* view);
private:
    std::pair<bool, double> zoomImpl(bool bigOrSmall) noexcept;
    Config m_config{};
};
