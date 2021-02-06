#include <QApplication>
#include <QTimer>
#include <iostream>
#include "QColorRect.hpp"


//http://jefftrull.github.io/qt/c++/coroutines/2018/07/21/coroutines-and-qt.html
#include <coroutine>

struct Task {
    struct promise_type;

    Task(promise_type& p)
        :coro_(std::coroutine_handle<promise_type>::from_promise(p))
    {};

    Task(Task const&) = delete;
    Task(Task&& other) noexcept
        :coro_(std::exchange(other.coro_, nullptr))
    {};

    ~Task() noexcept {
        if (coro_) {
            coro_.destroy();
        }
    }

    struct promise_type
    {
        auto initial_suspend() const noexcept {
            return std::suspend_never();
        }
        auto final_suspend() const noexcept {
            return std::suspend_never();
        }
        Task get_return_object() {
            return Task(*this);
        }
        void return_void() const noexcept {};
        void unhandled_exception() {};
    };
private:
    std::coroutine_handle<promise_type> coro_;
};

namespace v0
{
    struct AwaitableSignal
    {
        template<typename U, typename... Args>
        AwaitableSignal(U* obj, void(U::* m)(Args...))
        {
            conn_ = QObject::connect(obj, m, [&]() {
                QObject::disconnect(conn_);
                coro_.resume();
                });
        };

        struct awaiter {
            awaiter(AwaitableSignal* arg)
                :awaitable_(arg) {};

            bool await_ready() const noexcept {
                return false;
            }

            void await_suspend(std::coroutine_handle<> handler) noexcept {
                awaitable_->coro_ = handler;
            }

            void  await_resume() noexcept {};
        private:
            AwaitableSignal* awaitable_ = nullptr;
        };

        awaiter operator co_await() {
            return awaiter{ this };
        }
    private:
        QMetaObject::Connection conn_;
        std::coroutine_handle<> coro_;
    };
}


namespace v1
{
    template<typename R, typename... Args>
    struct AwaitableSignalSlot;

    template<>
    struct AwaitableSignalSlot<void>
    {
        auto operator()(QMetaObject::Connection& conn,
            std::coroutine_handle<>& handle)
        {
            return [&conn, &handle]() {
                QObject::disconnect(conn);
                handle.resume();
            };
        };
    };

    template<typename Arg>
    struct AwaitableSignalSlot<Arg, Arg>
    {
        auto operator()(QMetaObject::Connection& conn,
            Arg& result,
            std::coroutine_handle<>& handle)
        {
            return [&conn,&result, &handle](Arg arg) {
                QObject::disconnect(conn);
                result = arg;
                handle.resume();
            };
        };
    };

    template<typename... Args>
    struct AwaitableSignalSlot<std::tuple<Args...>, Args...>
    {
        auto operator()(QMetaObject::Connection& conn,
            std::tuple<Args...>& result,
            std::coroutine_handle<>& handle)
        {
            return [&conn,&result, &handle](Args... args) {
                QObject::disconnect(conn);
                result = std::make_tuple(args...);
                handle.resume();
            };
        };
    };


    template<typename T, typename R>
    struct AwaitableSignalBase {
        template<typename U, typename... Args>
        AwaitableSignalBase(U* obj, void(U::* m)(Args...),
            std::coroutine_handle<>& handle)
        {
            conn_ = QObject::connect(obj, m,
                AwaitableSignalSlot<R, Args...>()(
                    conn_,
                    args_,
                    handle
                    ));
        };
    protected:
        R  args_;
        QMetaObject::Connection conn_;
    };

    template<typename T>
    struct AwaitableSignalBase<T, void> {
        template<typename U, typename... Args>
        AwaitableSignalBase(U* obj, void(U::* m)(Args...),
            std::coroutine_handle<>& handle)
        {
            conn_ = QObject::connect(obj, m,
                AwaitableSignalSlot<void>()(
                    conn_,
                    handle
                    ));
        };
    protected:
        QMetaObject::Connection conn_;
    };

    
    template<typename T>
    struct ResultTypeImpl
    {
        using type = T;
    };

    template<typename T>
    struct ResultTypeImpl<std::tuple<T>>
    {
        using type = T;
    };

    template<>
    struct ResultTypeImpl<std::tuple<>>
    {
        using type = void;
    };

    //https://stackoverflow.com/questions/18366398/filter-the-types-of-a-parameter-pack
    template<typename...>
    struct Filter;

    template<>
    struct Filter<> {
        using type = std::tuple<>;
    };

    template<typename, typename >
    struct FilterHelpr;
    template<typename T, typename ... Ts>
    struct FilterHelpr<T, std::tuple<Ts...>>
    {
        using type = std::tuple<T, Ts...>;
    };

    template<typename T, typename... Ts>
    struct Filter<T, Ts...>
    {
        using type = std::conditional_t<!std::is_empty_v<T>,
            typename FilterHelpr<T, typename Filter<Ts...>::type>::type,
            typename Filter<Ts...>::type>;
    };

    template<typename F>
    struct Impl;

    template<typename R, typename C, typename... Args>
    struct Impl<R(C::*)(Args...)> {
        using type = typename ResultTypeImpl<typename Filter<Args...>::type>::type;
        using class_t = C;
    };

    template<typename F, typename R = typename Impl<F>::type>
    struct AwaitableSignal :AwaitableSignalBase<AwaitableSignal<F, R>, R>
    {
#if 0
        using obj_t = typename Impl<F>::class_t;
        AwaitableSignal(obj_t* o, F m)
            :AwaitableSignalBase<AwaitableSignal, R>(o, m, coro_)
        {};
#endif
        template<typename U>
        AwaitableSignal(U* o, F m)
            :AwaitableSignalBase<AwaitableSignal, R>(o, m, coro_)
        {};

        struct awaiter {
            awaiter(AwaitableSignal* arg)
                :awaitable_(arg) {};

            bool await_ready() const noexcept {
                return false;
            }

            void await_suspend(std::coroutine_handle<> handler) noexcept {
                awaitable_->coro_ = handler;
            }

            template<typename T = R>
            std::enable_if_t<std::is_same_v<T, void>, void>
                await_resume() noexcept {};

            template<typename T = R>
            std::enable_if_t<!std::is_same_v<T, void>, R>
                await_resume() noexcept {
                return awaitable_->args_;
            };
        private:
            AwaitableSignal* awaitable_ = nullptr;
        };

        awaiter operator co_await() {
            return awaiter{ this };
        }
    private:
        std::coroutine_handle<> coro_;
    };

    template<typename T, typename U, typename... Args>
    AwaitableSignal(T* o, void(U::*)(Args...))->AwaitableSignal<void(U::*)(Args...)>;
}

Task changeColor(ColorRect* view, QTimer* timer)
{
    while (true) {
        co_await v1::AwaitableSignal(timer, &QTimer::timeout);
        view->changeColor();
    }
}

Task drawLine(ColorRect* view)
{
    while (true) {
        QPointF first_point = co_await v1::AwaitableSignal(view, &ColorRect::click);
        QPointF second_point = co_await v1::AwaitableSignal(view, &ColorRect::click);
        view->setLine(first_point, second_point);
    }
}

Task report(ColorRect* view)
{
    while (true) {
        auto [p1, p2] = co_await v1::AwaitableSignal(view, &ColorRect::lineCreated);
        std::cout << "we draw a line from (";
        std::cout << p1.x() << ", " << p1.y() << ") to (";
        std::cout << p2.x() << ", " << p2.y() << ")\n";
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    ColorRect view;
    view.setWindowTitle("Color Cycler");
    view.show();
    QTimer* timer = new QTimer(&app);
    timer->start(500);

    auto t1 = changeColor(&view,timer);
    auto t2 = drawLine(&view);
    auto t3 = report(&view);
    return app.exec();
}
