# 单向数据流模式在节点编辑器中的应用样例

之前介绍过的单向数据流架构模式非常简洁易懂,能够很方便地支持界面与业务逻辑解耦,在可测试性方面表现不俗.这里结合之前的节点编辑器平移与缩放实现来演示一下在真实场景中该如何使用.

## 架构模式的支持库

在单向数据流架构模式中,有以下要素:

- `Store`:模型
- `View`:视图
- `Action`:动作
- `Dispatcher`:分发器

通常我们会使用`MVC`模式,对于`Store`和`View`基本上都会做较好的拆分,而`Action`就是普通的结构体,那么应用该架构模式就需要提供`Dispatcher`的实现.

`Dispatcher`本质上可以理解为事件处理,针对事件处理通常有两种模式:

1. 立即处理`dispatch`
2. 延后处理`post`

事件处理包含三要素:

1. 事件及其队列
2. 事件处理函数
3. 分发动作

这里使用`C++17`的`std::any`来存储任意的事件`Action`,事件处理函数的原型如下:

```C++
using Handler = void(std::any&&);
```

那么很快我们就能写出如下简易实现:

```C++
#pragma once

#include <queue>
#include <functional>
#include <map>
#include <string>
#include <any>

//https://facebook.github.io/flux/docs/overview

class Dispatcher
{
    std::queue<std::any> m_queue;
    std::map<std::string, 
        std::function<void(std::any&&)>> m_handlers;
public:
    Dispatcher() = default;

    /// @brief 注册action响应实现
    /// @tparam Fn 
    /// @param key 
    /// @param fn 
    template<typename Fn>
    void registerHandler(std::string const& key,
        Fn&& fn)
    {
        m_handlers[key] = std::move(fn);
    }

    /// @brief 注册action响应实现
    /// @tparam Fn 
    /// @param key 
    /// @param fn 
    template<typename Fn>
    void registerHandler(std::string const& key,
        Fn const& fn)
    {
        m_handlers[key] = fn;
    }

    /// @brief action投递
    /// @tparam T 
    /// @param v 
    /// @return 
    template<typename T>
    std::enable_if_t<!std::is_same<std::remove_cv_t<T>, std::any>::value>
        post(T const& v) {
        m_queue.push(v);
    }

    /// @brief action投递
    /// @tparam T 
    /// @param v 
    /// @return 
    template<typename T>
    std::enable_if_t<!std::is_same<std::remove_cv_t<T>, std::any>::value>
        post(T&& v) {
        m_queue.push(std::move(v));
    }
protected:
    /// @brief 分发处理,直到待处理action为空
    /// @param v 
    void dispatchImpl(std::any&& v) {
        m_queue.emplace(std::move(v));
        while (!m_queue.empty()) {
            auto& v = m_queue.front();
            for (auto&[k,h] : m_handlers) {
                if (!v.has_value())
                    continue;
                h(std::move(v));
            }
            m_queue.pop();
        }
    }
public:
    /// @brief 分发action
    /// @tparam T 
    /// @param v 
    /// @return 
    template<typename T>
    std::enable_if_t<!std::is_same<std::remove_cv_t<T>, std::any>::value>
        dispatch(T const& v) {
        dispatchImpl(v);
    }

    /// @brief 分发action
    /// @tparam T 
    /// @param v 
    /// @return 
    template<typename T>
    std::enable_if_t<!std::is_same<std::remove_cv_t<T>, std::any>::value>
        dispatch(T&& v) {
        dispatchImpl(std::move(v));
    }
};
```

通常只需要两步就能够正常运行:

1. 使用`registerHandler`注册事件处理函数
2. 使用`dispatch`分发`Action`

例如:

```C++
struct ReportAction{
    std::string msg;
};

int main(int argc,int argv){
    Dispatcher dispatcher;
    dispatcher.registerHandler("ReportActionHandler",
        [](std::any&& v){
		if(ReportAction* action = std::any_cast<ReportAction>(&v);action){
            std::cout<<action->msg<<"\n";
        }
    });
    
    dispatcher.dispatch(ReportAction{"liff.engineer@gmail.com"});
    return 0;
}
```

但是如果你希望和界面解耦,可能会将事件处理拆分成以下步骤:

1. 根据事件处理业务逻辑,与界面无依赖
2. 构造界面刷新`Action`,与界面无依赖
3. 界面刷新事件处理,与界面有依赖

这时就需要用到`post`接口,在步骤2中`post`界面刷新`Action`,从而触发后续界面刷新操作,这个会在后续示例中展示.

## 整体设计

`View`可以直接使用`Qt`提供的`QGraphicsView`,`Store`则采用简单的结构体即可,这里需要解决一个麻烦点儿的问题:

- 如何触发`Action`

这里可以通过为`QGraphicsView`提供`eventFilter`来截获事件,并向`Dispatcher`发出`Action`.以此来避免派生自`QGraphicsView`来实现新的类.

由于之前的平移实现需要用到`QGraphicsView`的`mousePressEvent`和`mouseReleaseEvent`两个保护成员方法为简单起见,使用一个简单的派生来公开这两个方法.这样代码骨架如下:

```C++
//视图View
class GraphicsViewImpl :public QGraphicsView
{
public:
    using QGraphicsView::QGraphicsView;

    void mousePressEvent(QMouseEvent* event) override {
        return QGraphicsView::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        return QGraphicsView::mouseReleaseEvent(event);
    }
};

//视图向Dispatcher发出Action的触发器
class GraphicsViewEventFilter :public QObject
{
    GraphicsViewImpl* m_target = nullptr;
    Dispatcher* m_dispatcher = nullptr;
public:
    GraphicsViewEventFilter(GraphicsViewImpl* target,
        Dispatcher* dispatcher,
        QObject* parent = nullptr)
        :QObject(parent),m_target(target),m_dispatcher(dispatcher) {
        if (m_target == nullptr || m_dispatcher == nullptr)
            throw std::invalid_argument("invalid ctor argument");
    };
	//TODO
};



int main(int argc, char **argv)
{
    QApplication app(argc, argv);    
    //分发器
    Dispatcher   dispatcher;
	
    //视图
    GraphicsViewImpl appView;
    GraphicsViewEventFilter filter{ &appView,&dispatcher,&app };
    //注册事件过滤器
    appView.installEventFilter(&filter);
    appView.viewport()->installEventFilter(&filter);//滚轮等事件处理对应viewport

    appView.show();
    return app.exec();
}
```

然后我们初始化一下视图(之前在派生类中实现,现在不需要了):

```C++
int main(int argc, char **argv)
{
    //...
    
    
    {//初始化
        double w = 64000.0;
        double h = 32000.0;

        appView.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        appView.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        appView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        appView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        appView.setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        appView.setDragMode(QGraphicsView::RubberBandDrag);

        appView.setScene(new QGraphicsScene(&appView));
        appView.setSceneRect(-w / 2, -h / 2, w, h);
    }
    {//添加几个graphicsItem
        appView.scene()->addSimpleText(QString(u8"liff.engineer@gmail.com"));
        appView.scene()->addRect(-200.0, -150.0, 400.0, 300.0);
    }
    appView.show();
    return app.exec();
}
```

之后,我们就可以来实现平移和缩放了.

## 平移实现

平移动作是由起始和结束构成的,由于我们的实现和视图紧绑定,这里定义`Action`如下:

```C++
struct PanBeginAction {
    GraphicsViewImpl* target = nullptr;
    QMouseEvent* event;
};

struct PanEndAction {
    GraphicsViewImpl* target = nullptr;
    QMouseEvent* event;
};
```

对应的`Action`处理函数如下:

```C++
void PanBeginHandler(PanBeginAction action)
{
    auto [view, event] = action;
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event->localPos(), event->screenPos(),
        Qt::LeftButton, Qt::NoButton, event->modifiers());
    view->mouseReleaseEvent(&releaseEvent);

    //切换到滚动模式来完成平移动作
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    QMouseEvent fakeEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
        event->buttons() | Qt::LeftButton, event->modifiers());
    view->mousePressEvent(&fakeEvent);
}

void PanEndHandler(PanEndAction action)
{
    auto [view, event] = action;
    //鼠标中键释放后切换回原有模式
    QMouseEvent fakeEvent(event->type(), event->localPos(), event->screenPos(), Qt::LeftButton,
        event->buttons() & ~Qt::LeftButton, event->modifiers());
    view->mouseReleaseEvent(&fakeEvent);
    view->setDragMode(QGraphicsView::RubberBandDrag);
}
```

然后实现`eventFilter`用来触发`Action`:

```C++
class GraphicsViewEventFilter :public QObject
{
    GraphicsViewImpl* m_target = nullptr;
    Dispatcher* m_dispatcher = nullptr;
public:
	//...
    
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched != m_target && watched != m_target->viewport()) {
            return QObject::eventFilter(watched, event);
        }
        if (event->type() == QEvent::MouseButtonPress) {
            auto e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::MiddleButton) {
                //平移开始
                m_dispatcher->dispatch(PanBeginAction{m_target,e});
                return true;
            }
            return false;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::MiddleButton) {
                //平移结束
                m_dispatcher->dispatch(PanEndAction{ m_target,e });
                return true;
            }
            return false;
        }
        return false;
    }
};
```

然后向`Dispatcher`注册`Action`处理函数:

```C++
int main(int argc, char **argv)
{
    //...
    //平移开始Action处理
    dispatcher.registerHandler("GraphicsViewPanBeginHandler",
        [&](std::any&& v) {
            if (PanBeginAction* action = std::any_cast<PanBeginAction>(&v); action) {
                PanBeginHandler(*action);
            }
        });
    //平移结束Action处理
    dispatcher.registerHandler("GraphicsViewPanEndHandler",
        [&](std::any&& v) {
            if (PanEndAction* action = std::any_cast<PanEndAction>(&v); action) {
                PanEndHandler(*action);
            }
        });
    //...
};    
```

## 缩放实现

首先定义缩放的`Store`,简单起见,缩放实现也同步写到`Store`中:

```C++

/// @brief 缩放设置
struct ZoomSetting
{
    double zoomInFactor = 1.25;
    bool zoomClamp = true;
    double zoom = 10.0;
    double zoomStep = 1.0;
    std::pair<double, double> zoomRange = { 0, 20 };

    /// @brief 缩放
    /// @param bigOrSmall 
    /// @return 
    std::pair<bool,double> zoomImpl(bool bigOrSmall) noexcept
    {
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

        return std::make_pair(zoomClamp == false || clamped == false,
            zoomFactor);
    }
};
```

然后定义缩放`Action`和缩放实现`Action`:

```C++
/// @brief 缩放动作
struct ZoomAction
{
    bool bigOrSmall = true;
};

/// @brief 缩放实现动作
struct ZoomImplAction
{
    double factor;
};
```

然后是对应的处理函数:

```C++
/// @brief 缩放处理
/// @param cfg 
/// @param action 
void ZoomHandler(Dispatcher& dispatcher,ZoomSetting& cfg, ZoomAction action)
{
    auto [result,factor] = cfg.zoomImpl(action.bigOrSmall);
    if (result) {
        dispatcher.post(ZoomImplAction{factor});
    }
}

/// @brief 视图缩放实现
/// @param view 
/// @param action 
void ZoomImplHandler(QGraphicsView* view, ZoomImplAction action) {
    if (view) {
        view->scale(action.factor, action.factor);
    }
}
```

注意`ZoomAction`只是用来触发模型刷新,然后发出视图刷新信号,真正的视图刷新在`ZoomImplHandler`.

这时就可以调整`eventFilter`实现来触发:

```C++
class GraphicsViewEventFilter :public QObject
{
public:    
    bool eventFilter(QObject* watched, QEvent* event) override {
        //...
        if (event->type() == QEvent::Wheel) {
            //在滚动模式下不缩放,所以不拦截事件
            if (m_target->dragMode() == QGraphicsView::ScrollHandDrag)
                return false;
            auto e = static_cast<QWheelEvent*>(event);
            m_dispatcher->dispatch(ZoomAction{ e->angleDelta().y() > 0});
            return true;
        }
        if (event->type() == QEvent::KeyPress)
        {
            auto e = static_cast<QKeyEvent*>(event);
            if (e->matches(QKeySequence::ZoomIn)) {
                m_dispatcher->dispatch(ZoomAction{ true });
                return true;
            }
            else if (e->matches(QKeySequence::ZoomOut)) {
                m_dispatcher->dispatch(ZoomAction{ false });
                return true;
            }
            return false;
        }
		//...
        return false;
    }
};
```

最后向`Dispatcher`注册处理函数:

```C++
int main(int argc, char **argv)
{
    //...
    
    //注意缩放是完全与视图无关的
    dispatcher.registerHandler("GraphicsViewZoomHandler",
        [&](std::any&& v) {
            if (ZoomAction* action = std::any_cast<ZoomAction>(&v); action) {
                ZoomHandler(dispatcher, zoomSetting, *action);
            }
        });

    //...
	
    //缩放实现需要在视图构造完成后注册
    dispatcher.registerHandler("GraphicsViewZoomImplHandler",
        [&](std::any&& v) {
            if (ZoomImplAction* action = std::any_cast<ZoomImplAction>(&v); action) {
                ZoomImplHandler(&appView, *action);
            }
        });
    //...
}
```

至此缩放功能就实现了.

## 总结

在之前的实现中,我们通过提供`GraphicsView`完成了所有实现,在功能简单的情况下是没有问题的,随着`GraphicsView`功能复杂度越来越高,这种写法就会带来理解、调试、维护上的困难.

通过明确的职责划分,严格的数据/事件流向处理,我们可以把软件的功能拆分成一个个互相独立的定义和函数实现,能够任意拼接组合及复用,当需要调整某部分实现时,也无需派生,只需要简单地替换掉处理函数即可;同时我们可以看到,某些部分是可以完全与界面解耦的,这使得可测试性也有了保障.















