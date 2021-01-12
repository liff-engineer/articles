# 节点编辑器的画布缩放与平移实现

对于软件开发来讲,一方面业务场景越来越复杂,另外一方面用户希望有更多的定制能力,这就使得低代码/无代码开发方式越来越流行.

对于某些开发者来讲,低代码/无代码开发并不是什么新鲜事.在电子、测控、设计、建筑、游戏等行业均采用一种被称为可视化编程的交互方式来完成或者扩展业务场景,其主要载体就是节点编辑器.

节点编辑器核心要素为节点及其构成的数据流.开发者提供不同类型的功能节点,使用者基于这些功能节点构建数据数据处理流程,即一张有向图,从而完成功能开发与定制.在游戏行业甚至能够编译为C++代码来运行.

这里基于`Qt`的`GraphicsView`框架来展示节点编辑器主要交互界面-画布的缩放和平移实现逻辑.实现方式参考自[Node Editor in Python Tutorial Series](https://www.blenderfreak.com/tutorials/node-editor-tutorial-series/).

## 缩放实现逻辑

`GraphicsView`框架提供的画布视图`QGraphicsView`自身带有转换矩阵,缩放通过调用`scale`即可完成.我们首先来看一下具体的需求:

1. 通过鼠标滚轮放大缩小视图
2. 缩放时基于当前鼠标点
3. 缩放有上下限,不能无限缩放

### 缩放实现

这里通过`zoomImpl(bigOrSmall)`来实现缩放,缩放因子为`zoomInFactor`,当前缩放值为`zoom`,步进为`zoomStep`,缩放范围为`zoomRange`,并提供`zoomClamp`来控制是否是能缩放上下限.

那么`zoomImpl`的`C++`实现如下:

```C++
void GraphicsView::zoomImpl(bool bigOrSmall)
{
    //计算缩放因子并更新当前缩放值
    auto zoomOutFactor = 1 / zoomInFactor;
    double zoomFactor = zoomOutFactor;
    if (bigOrSmall)
    {
        zoomFactor = zoomInFactor;
        zoom += zoomStep;
    }
    else
    {
        zoomFactor = zoomOutFactor;
        zoom -= zoomStep;
    }
    
    //缩放范围限制
    auto clamped = false;
    if (zoom < zoomRange.first)
    {
        zoom = zoomRange.first;
        clamped = true;
    }
    else if (zoom > zoomRange.second)
    {
        zoom = zoomRange.second;
        clamped = true;
    }
	
    //进行缩放
    if (zoomClamp == false || clamped == false)
    {
        this->scale(zoomFactor, zoomFactor);
    }
}
```

对应的`Python`为:

```python
def zoomImpl(self, bigOrSmall: bool):
    zoomOutFactor = 1 / self.zoomInFactor
    zoomFactor = zoomOutFactor
    if bigOrSmall:
        zoomFactor = self.zoomInFactor
        self.zoom += self.zoomStep
    else:
        zoomFactor = zoomOutFactor
        self.zoom -= self.zoomStep
    clamped = False

    if self.zoom < self.zoomRange[0]:
        self.zoom, clamped = self.zoomRange[0], True
    if self.zoom > self.zoomRange[1]:
        self.zoom, clamped = self.zoomRange[1], True

    if not clamped or self.zoomClamp is False:
        self.scale(zoomFactor, zoomFactor)
```

### 快捷键处理

放大缩小有标准的快捷键,这就需要处理键盘事件,在`C++`中实现如下:

```C++
void GraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::ZoomIn))
    {
        zoomImpl(true);
    }
    else if (event->matches(QKeySequence::ZoomOut))
    {
        zoomImpl(false);
    }
    else
    {
        QGraphicsView::keyPressEvent(event);
    }
}
```

在`Python`中实现如下:

```python
def keyPressEvent(self, event):
    if event.matches(QKeySequence.ZoomIn):
        self.zoomImpl(True)
    elif event.matches(QKeySequence.ZoomOut):
        self.zoomImpl(False)
    else:
        super().keyPressEvent(event)
```

### 鼠标滚轮缩放

默认的`QGraphicsView`包含水平和垂直滚动条,如果不调整,则鼠标滚轮操作会进行视图平移,所以需要关闭滚动条,然后调整滚轮事件.这里需要注意,如果视图处于`ScrollHandDrag`模式则不要处理滚轮事件,否则平移和缩放会混合处理,产生视图滑动.

另外,需要在鼠标点执行缩放可以通过`setTransformationAnchor`设置转换基于鼠标点.

初始化`GraphicsView`时的`C++`实现为:

```C++
GraphicsView::GraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    //...
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);    //关闭水平滚动条
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//关闭垂直滚动条
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);//基于鼠标点缩放
    //...
}
```

鼠标滚轮事件处理的`C++`实现为:

```C++
void GraphicsView::wheelEvent(QWheelEvent *event)
{
    if (dragMode() == QGraphicsView::ScrollHandDrag)
        return;
    return zoomImpl(event->angleDelta().y() > 0);
}
```

对应的`Python`实现为:

```python
class GraphicsView(QGraphicsView):

    def __init__(self, parent=None):
        super().__init__(parent)

        # ...
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setTransformationAnchor(QGraphicsView.AnchorUnderMouse)
        # ...
        
    def wheelEvent(self, event):
        if self.dragMode() == QGraphicsView.ScrollHandDrag:
            return
        return self.zoomImpl(event.angleDelta().y() > 0)     
```

### 默认配置

这里提供一些默认的缩放配置供参考,`C++`代码如下:

```C++
class GraphicsView : public QGraphicsView
{
public:
    explicit GraphicsView(QWidget *parent = nullptr);
	//...
private:
    double zoomInFactor = 1.25;
    bool zoomClamp = true;
    double zoom = 10.0;
    double zoomStep = 1.0;
    std::pair<double, double> zoomRange = {0, 20};
};
```

`Python`代码如下:

```python
class GraphicsView(QGraphicsView):

    def __init__(self, parent=None):
        super().__init__(parent)

        # 画布视图尺寸
        self.w = 64000.0
        self.h = 32000.0

        # 缩放相关
        self.zoomInFactor = 1.25
        self.zoomClamp = True
        self.zoom = 10
        self.zoomStep = 1
        self.zoomRange = [0, 20]
```



## 平移实现逻辑

平移实现则没有那么直接,我们可以利用`QGraphicsView`提供的`ScrollHandDrag`交互模式来完成平移实现,方式如下:

1. 平移开始时切换到`ScrollHandDrag`模式
2. 模拟鼠标左键按下事件,交由`QGraphicsView`处理平移
3. 平移结束时模拟鼠标左键弹起事件,交由`GraphicsView`处理平移
4. 切换回`RubberBandDrag`模式

### 平移开始与结束实现

`C++`代码如下:

```C++
void GraphicsView::panBeginImpl(QMouseEvent *event)
{
    auto releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, event->localPos(), event->screenPos(),
                                        Qt::LeftButton, Qt::NoButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(releaseEvent);
    //切换到滚动模式来完成平移动作
    setDragMode(QGraphicsView::ScrollHandDrag);
    auto fakeEvent = new QMouseEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
                                     event->buttons() | Qt::LeftButton, event->modifiers());
    QGraphicsView::mousePressEvent(fakeEvent);
}
void GraphicsView::panEndImpl(QMouseEvent *event)
{
    //鼠标中键释放后切换回原有模式
    auto fakeEvent = new QMouseEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
                                     event->buttons() & ~Qt::LeftButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(fakeEvent);
    setDragMode(QGraphicsView::RubberBandDrag);
}
```

对应的`Python`代码如下:

```python
def panBeginImpl(self, event):
    releaseEvent = QMouseEvent(QEvent.MouseButtonRelease, event.localPos(), event.screenPos(),
                                Qt.LeftButton, Qt.NoButton, event.modifiers())
    super().mouseReleaseEvent(releaseEvent)
    self.setDragMode(QGraphicsView.ScrollHandDrag)
    fakeEvent = QMouseEvent(event.type(), event.localPos(), event.screenPos(),
                            Qt.LeftButton, event.buttons() | Qt.LeftButton, event.modifiers())
    super().mousePressEvent(fakeEvent)

def panEndImpl(self, event):
    fakeEvent = QMouseEvent(event.type(), event.localPos(), event.screenPos(),
                            Qt.LeftButton, event.buttons() & ~Qt.LeftButton, event.modifiers())
    super().mouseReleaseEvent(fakeEvent)
    self.setDragMode(QGraphicsView.RubberBandDrag)
```

### 鼠标中键平移

首先设置`dragMode`:

```C++
GraphicsView::GraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    //...
    setDragMode(QGraphicsView::RubberBandDrag);
	//...
}
```

然后重载鼠标事件处理:

```C++
void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        return panBeginImpl(event);
    }
    QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        return panEndImpl(event);
    }
    QGraphicsView::mouseReleaseEvent(event);
}
```

对应的`Python`代码如下:

```python
class GraphicsView(QGraphicsView):

    def __init__(self, parent=None):
        super().__init__(parent)
		##...
        self.setDragMode(QGraphicsView.RubberBandDrag)

        self.setScene(QGraphicsScene())
        self.setSceneRect(-self.w/2, -self.h/2, self.w, self.h)
    
    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton:
            return self.panBeginImpl(event)
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MiddleButton:
            return self.panEndImpl(event)
        super().mouseReleaseEvent(event)
```

## 样例

默认为`GraphicsView`提供`QGraphicsScene`:

```C++
GraphicsView::GraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::RubberBandDrag);

    setScene(new QGraphicsScene(this));
    setSceneRect(-w / 2, -h / 2, w, h);
}
```

示例如下:

```C++
#include <QtWidgets/QApplication>
#include "GraphicsView.hpp"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    GraphicsView appView{nullptr};
    {
        appView.scene()->addSimpleText(QString(u8"liff.engineer@gmail.com"));
        appView.scene()->addRect(-200.0, -150.0, 400.0, 300.0);
    }
    appView.show();
    return app.exec();
}
```

对应的`Python`示例如下:

```python
if __name__ == "__main__":
    app = QApplication(sys.argv)
    appView = GraphicsView()
    appView.scene().addSimpleText('liff.engineer@gmail.com')
    appView.scene().addRect(-200, -150, 400, 300)
    appView.show()
    sys.exit(app.exec_())
```