#pragma once

#include <QtWidgets/QGraphicsView>

class GraphicsView : public QGraphicsView
{
public:
    explicit GraphicsView(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void zoomImpl(bool bigOrSmall);
    void panBeginImpl(QMouseEvent *event);
    void panEndImpl(QMouseEvent *event);

private:
    double zoomInFactor = 1.25;
    bool zoomClamp = true;
    double zoom = 10.0;
    double zoomStep = 1.0;
    std::pair<double, double> zoomRange = {0, 20};

private:
    //画布尺寸
    double w = 64000.0;
    double h = 32000.0;
};