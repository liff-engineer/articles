#pragma once

#include <QtWidgets/QGraphicsView>

class PanActor;
class GraphicsView :public QGraphicsView{
public:
    using QGraphicsView::QGraphicsView;
private:
    friend class PanActor;
};
