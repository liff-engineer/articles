#include <QMouseEvent>
#include <QPainter>
#include "QColorRect.hpp"

ColorRect::ColorRect(QWidget* parent) :
    QWidget{ parent },
    colorList{ {"#111111", "#113311",
                "#111133", "#331111",
                "#333311", "#331133",
                "#661111", "#116611",
                "#111166", "#663311",
                "#661133", "#336611",
                "#331166", "#113366"} },
    curColor{ 0 }
{}

void ColorRect::setColor(std::string const& col)
{
    setStyleSheet(("background-color:" + col).c_str());
}

void ColorRect::changeColor()
{
    curColor++;
    if (curColor >= colorList.size())
    {
        curColor = 0;
    }
    setColor(colorList[curColor]);
}

void ColorRect::mousePressEvent(QMouseEvent* e)
{
    emit click(e->windowPos());
}

void ColorRect::paintEvent(QPaintEvent*)
{
    if (line) {
        QPainter painter(this);
        painter.setPen(QPen{ QColor{"yellow"} });
        painter.drawLine(*line);
    }
}

void ColorRect::setLine(QPointF p1, QPointF p2)
{
    line = QLineF{ p1, p2 };
    emit lineCreated(p1, p2);
}
