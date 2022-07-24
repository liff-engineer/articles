#include "ApplicationView.hpp"
#include "GraphicsView.hpp"

#include "Command/ViewCommand.hpp"
#include "Command/PointDrawCommand.hpp"
#include "Command/CommandIds.hpp"

#include <QtWidgets/QToolBar>

namespace
{
    void InitializeView(GraphicsView& view)
    {
        {//初始化
            double w = 64000.0;
            double h = 32000.0;

            view.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
            view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            view.setDragMode(QGraphicsView::RubberBandDrag);

            view.setScene(new QGraphicsScene(&view));
            view.setSceneRect(-w / 2, -h / 2, w, h);
        }
        {//添加几个graphicsItem
            view.scene()->addSimpleText(QString(u8"liff.engineer@gmail.com"));
            view.scene()->addRect(-200.0, -150.0, 400.0, 300.0);
        }
    }
}
ApplicationView::ApplicationView(QWidget* parent, Qt::WindowFlags flags)
    :QMainWindow(parent,flags)
{
    m_runner = std::make_unique<CommandRunner>(&m_context);

    auto view = new GraphicsView{};
    InitializeView(*view);
    setCentralWidget(view);

    auto toolBar = addToolBar(u8"默认工具栏");
    {
        auto action = toolBar->addAction(u8"查看");
        QObject::connect(action, &QAction::triggered, [&,view]() {
            m_runner->runCommand(ViewCommandCode);
            });
    }
    {
        auto action = toolBar->addAction(u8"绘制");
        QObject::connect(action, &QAction::triggered, [&, view]() {
            m_runner->runCommand(PointDrawCommandCode);
            });
    }

    m_context.attach<QGraphicsView>(view);
    {
        m_runner->registerCommand(ViewCommandCode,
            std::make_unique<ViewCommand>());
        m_runner->registerCommand(PointDrawCommandCode,
            std::make_unique<PointDrawCommand>());

        m_runner->setDefaultCommand(ViewCommandCode);
    }
    m_runner->runCommand(ViewCommandCode);
    //m_runner->runCommand(DrawPointCommandCode);
}
