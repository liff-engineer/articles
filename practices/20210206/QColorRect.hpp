#pragma once

#include <optional>
#include <string>
#include <QWidget>
#include <QLine>

class ColorRect : public QWidget
{
    Q_OBJECT

public:
    ColorRect(QWidget* parent = 0);

public slots:
    void changeColor();
    void setLine(QPointF, QPointF);

signals:
    void click(QPointF);
    void lineCreated(QPointF, QPointF);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void setColor(std::string const&);
    std::vector<std::string> colorList;
    std::size_t              curColor;
    std::optional<QLineF>    line;
};
