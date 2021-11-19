#pragma once
#include <type_traits>
#include <memory>
#include <cassert>
#include <string_view>
#include <functional>

//编译期类型ID机制可参考 https://stackoverflow.com/a/56600402
//这里只是演示
using type_code = std::string_view;

template<typename T>
constexpr type_code type_code_of() noexcept {
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

/// @brief 消息发布者
class publisher {

    /// @brief 使用NVI的消息处理基类
    struct message_handler_base {
        /// @brief 类型到void*的安全转换
        /// @tparam T 类型
        /// @param obj 对象
        /// @return 
        template<typename T>
        const void* address_of(const T& obj) {
            if constexpr (!std::is_polymorphic_v<T> || std::is_final_v<T>) {
                return &obj;
            }
            else
                return dynamic_cast<const void*>(&obj);
        }

        template<typename T>
        void handle(T const& msg) {
            handle_impl(address_of(msg), type_code_of<T>());
        }
    protected:
        virtual void handle_impl(const void* msg, type_code code) = 0;
    };

    /// @brief 由lambda构造的通用消息处理类
    /// @tparam Fn 消息处理函数
    /// @tparam T 消息类型
    template<typename Fn, typename T>
    class message_handler final :public message_handler_base {
        Fn m_action;
    public:
        explicit message_handler(Fn&& fn)
            :m_action(std::move(fn)) {};
    protected:
        void handle_impl(const void* msg, type_code code) override final {
            assert(code == type_code_of<T>());
            m_action(*static_cast<const T*>(msg));
        }
    };

    /// @brief 订阅存根
    struct stub {
        type_code code;
        std::weak_ptr<message_handler_base> handler;
    };
    std::vector<stub> m_stubs;
public:
    /// @brief 订阅存根,用来取消订阅
    using subscribe_stub = std::function<void()>;
    publisher() = default;

    /// @brief 发布消息
    /// @tparam T 消息类型
    /// @param msg 消息体
    template<typename T>
    void publish(T const& msg) {
        constexpr auto code = type_code_of<T>();
        for (auto& o : m_stubs) {
            if (o.code != code) continue;
            if (auto h = o.handler.lock(); h) {
                h->handle(msg);
            }
        }
    }

    /// @brief 信道存根
    /// @tparam T 消息类型
    template<typename T>
    struct channel_stub {
        publisher* owner{};

        /// @brief 订阅消息
        /// @tparam Fn 消息处理函数类型
        /// @param fn 消息处理函数
        /// @return 订阅存根
        template<typename Fn>
        subscribe_stub subsrcibe(Fn&& fn) {
            assert(owner != nullptr);

            auto handler = std::make_shared<message_handler<Fn, T>>(std::move(fn));
            owner->m_stubs.emplace_back(stub{ type_code_of<T>(),handler });
            return[h = std::move(handler)](){};
        }
    };

    /// @brief 获取指定消息类型的信道存根
    /// @tparam T 消息类型
    /// @return 信道存根
    template<typename T>
    channel_stub<T> channel() noexcept {
        return { this };
    }
};

/// @brief 订阅存根,用来管理信道生命周期,也可以用来取消订阅
class subscribe_stub {
public:
    subscribe_stub() = default;
    subscribe_stub(publisher::subscribe_stub&& stub)
    {
        m_stubs.emplace_back(std::move(stub));
    }
    subscribe_stub(std::vector<publisher::subscribe_stub>&& stubs)
        :m_stubs(std::move(stubs)) {};

    subscribe_stub(subscribe_stub const&) = delete;
    subscribe_stub& operator=(subscribe_stub const&) = delete;

    subscribe_stub(subscribe_stub&& other) noexcept
        :m_stubs(std::move(other.m_stubs)) {};

    subscribe_stub& operator=(subscribe_stub&& other) noexcept {
        if (std::addressof(other) != this) {
            unsubscribe();
            m_stubs = std::move(other.m_stubs);
        }
        return *this;
    }

    ~subscribe_stub() noexcept {
        try { unsubscribe(); }
        catch (...) {};
    }

    subscribe_stub& operator+=(publisher::subscribe_stub&& stub) noexcept {
        m_stubs.emplace_back(std::move(stub));
        return *this;
    }

    subscribe_stub& operator+=(subscribe_stub&& other) noexcept {
        if (std::addressof(other) != this) {
            for (auto& stub : other.m_stubs) {
                m_stubs.emplace_back(std::move(stub));
            }
            other.m_stubs.clear();
        }
        return *this;
    }

    void unsubscribe() {
        for (auto& h : m_stubs) {
            if (h) h();
        }
        m_stubs.clear();
    }
private:
    std::vector<publisher::subscribe_stub> m_stubs;
};

/// @brief 自定义消息源扩展点
/// @tparam T 消息源类型
/// @tparam E 使能
template<typename T, typename E = void>
struct subscribe_helper;

/// @brief 订阅辅助类,支持链式调用,可采用类型的on方法建立订阅
/// @tparam T 消息处理类
template<typename T>
class subscriber {
    T* m_obj{};
    std::vector<publisher::subscribe_stub> m_stubs;
public:
    explicit subscriber(T* obj) :m_obj(obj) { assert(m_obj != nullptr); };

    template<typename U, typename Fn>
    subscriber& subscribe(publisher::channel_stub<U> channel, Fn&& fn) {
        assert(channel.owner != nullptr);
        m_stubs.emplace_back(
            channel.subsrcibe(
                [obj = m_obj, h = std::move(fn)](auto arg){
            std::invoke(h, obj, arg);
        })
        );
        return *this;
    }

    template<typename U>
    subscriber& subscribe(publisher::channel_stub<U> channel) {
        return subscribe(channel, [](auto obj, auto arg) { obj->on(arg); });
    }

    template<typename U, typename Fn>
    subscriber& subscribe(publisher* o, Fn&& fn) {
        assert(o != nullptr);
        return subscribe(o->channel<U>(), std::move(fn));
    }

    template<typename... Us>
    subscriber& subscribe(publisher* o) {
        assert(o != nullptr);
        (subscribe(o->channel<Us>()),...);
        return *this;
    }

    template<typename U, typename Fn>
    subscriber& subscribe(U* source, Fn&& fn) {
        assert(source != nullptr);
        m_stubs.emplace_back(
            subscribe_helper<std::decay_t<U>>::subscribe(source,
                [obj = m_obj, h = std::move(fn)](auto arg){
            std::invoke(h, obj, arg);
        })
        );
        return *this;
    }

    template<typename U>
    subscriber& subscribe(U* source) {
        assert(source != nullptr);
        return subscribe(source, [](auto obj, auto arg) { obj->on(arg); });
    }

    operator subscribe_stub() noexcept {
        return subscribe_stub{ std::move(m_stubs) };
    }
};

template<typename T>
subscriber(T* obj)->subscriber<T>;

/// @brief 针对自定义消息源进行订阅的自由函数
/// @tparam T 消息源类型
/// @tparam Fn 消息处理函数类型
/// @param source 消息源
/// @param fn 消息处理函数
/// @return 订阅存根
template<typename T, typename Fn>
subscribe_stub subscribe(T* source, Fn&& fn) {
    return subscribe_stub(
        subscribe_helper<std::decay_t<T>>::subscribe(source, std::move(fn))
    );
}

/// @brief 针对发布者,基于信道订阅的自由函数
/// @tparam T 消息类型
/// @tparam Fn 消息处理函数类型
/// @param channel 信道
/// @param fn 消息处理函数
/// @return 订阅存根
template<typename T, typename Fn>
subscribe_stub subscribe(publisher::channel_stub<T> channel, Fn&& fn) {
    assert(channel.owner != nullptr);
    return subscribe_stub(channel.subsrcibe(std::move(fn)));
}
