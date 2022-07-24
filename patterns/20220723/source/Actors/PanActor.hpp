#pragma once

class QGraphicsView;
class QMouseEvent;

class PanActor{
public:
    void BeginPan(QGraphicsView* view,QMouseEvent* event);
    void EndPan(QGraphicsView* view, QMouseEvent* event);
};
