//辅助Actor编程模型的消息机制
// - 支持发布-订阅模式
// - 支持请求-回复模式
// - 支持组合式设计

#pragma once
#include <type_traits>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <cassert>

//编译期类型ID机制可参考 https://stackoverflow.com/a/56600402
//这里只是演示
using type_code = std::string_view;

template<typename... Ts>
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

/// @brief 消息中间人
class broker {

    /// @brief 消息处理基类
    struct message_handler_base {
        template<typename T>
        const void* address_of(const T& obj) {
            if constexpr (!std::is_polymorphic_v<T> || std::is_final_v<T>) {
                return &obj;
            }
            else
                return dynamic_cast<const void*>(&obj);
        }

        /// @brief 订阅消息的处理接口
        /// @tparam T 消息类型
        /// @param msg 消息
        template<typename T>
        void on(T const& msg) {
            handle_impl(address_of(msg),
                nullptr,
                type_code_of<T, void>()
            );
        }

        /// @brief 请求消息的处理接口
        /// @tparam T 请求参数类型
        /// @tparam R 回复类型
        /// @param msg 请求参数
        /// @param result 回复
        template<typename T, typename R>
        void reply(T const& msg, R& result) {
            handle_impl(address_of(msg),
                const_cast<void*>(address_of(result)),
                type_code_of<T, R>()
            );
        }

        /// @brief 无参数请求消息的处理接口
        /// @tparam R 回复类型
        /// @param result 回复
        template<typename R>
        void reply(R& result) {
            handle_impl(nullptr,
                const_cast<void*>(address_of(result)),
                type_code_of<void, R>()
            );
        }
    protected:
        virtual void handle_impl(const void* msg, void* result, type_code code) = 0;
    };

    /// @brief 接收lambda的消息处理类
    /// @tparam Fn 消息处理动作类型
    /// @tparam T 请求/消息类型
    /// @tparam R 回复类型
    template<typename Fn, typename T, typename R>
    class message_handler final :public message_handler_base {
        Fn m_action;
    public:
        explicit message_handler(Fn&& fn)
            :m_action(std::move(fn)) {};
    protected:
        void handle_impl(const void* msg, void* result, type_code code) override final {
            constexpr auto require_code = type_code_of<T, R>();
            assert(code == require_code);
            if constexpr (std::is_same_v<T, void>) {
                m_action(*static_cast<R*>(result));
            }
            else if constexpr (std::is_same_v<R, void>) {
                m_action(*static_cast<const T*>(msg));
            }
            else {
                m_action(*static_cast<const T*>(msg), *static_cast<R*>(result));
            }
        }
    };

    /// @brief 消息处理动作存根
    struct handler_stub {
        type_code code;
        std::weak_ptr<message_handler_base> handler;
    };

    std::vector<handler_stub> m_stubs;


    template<std::size_t I>
    struct priority_tag :priority_tag<I - 1> {};

    template<>
    struct priority_tag<0> {};

    /// @brief 发送请求,提供了回复合并类,高优先级
    template<typename T, typename Op>
    auto request_impl(T const& msg, Op& op, priority_tag<1>)
        ->decltype(op.value()) {
        constexpr auto code = type_code_of<T, Op::type>();
        for (auto& o : m_stubs) {
            if (o.code != code) continue;
            if (auto h = o.handler.lock(); h) {
                h->reply(msg, op.alloc());
            }
        }
        return op.value();
    }

    /// @brief 发送请求,提供回复结果引用,常规情况
    template<typename T, typename R>
    auto request_impl(T const& msg, R& result, priority_tag<0>)
        ->decltype(void()) {
        constexpr auto code = type_code_of<T, R>();
        for (auto& o : m_stubs) {
            if (o.code != code) continue;
            if (auto h = o.handler.lock(); h) {
                h->reply(msg, result);
            }
        }
    }

    /// @brief 请求回复,并提供了回复合并类,高优先级
    template<typename Op>
    auto request_impl(Op& op, priority_tag<1>)
        ->decltype(op.value()) {
        constexpr auto code = type_code_of<void, Op::type>();
        for (auto& o : m_stubs) {
            if (o.code != code) continue;
            if (auto h = o.handler.lock(); h) {
                h->reply(op.alloc());
            }
        }
        return op.value();
    }

    /// @brief 请求回复,提供回复结果引用,常规情况
    template<typename R>
    auto request_impl(R& result, priority_tag<0>)
        ->decltype(void()) {
        constexpr auto code = type_code_of<void, R>();
        for (auto& o : m_stubs) {
            if (o.code != code) continue;
            if (auto h = o.handler.lock(); h) {
                h->reply(result);
            }
        }
    }
public:
    /// @brief 消息处理动作存根单元,用来移除
    using action_stub_unit = std::function<void()>;

    broker() = default;

    /// @brief 发布消息
    /// @tparam T 消息类型
    /// @param msg 消息
    template<typename T>
    void publish(T const& msg) {
        constexpr auto code = type_code_of<T, void>();
        for (auto& o : m_stubs) {
            if (o.code != code) continue;
            if (auto h = o.handler.lock(); h) {
                h->on(msg);
            }
        }
    }

    /// @brief 发送请求并获取结果
    /// @tparam T 请求参数类型
    /// @tparam R 回复类型、或回复合并类类型
    /// @param msg 请求参数
    /// @param result 回复、或回复合并类
    /// @return 回复,如果提供了合并类则由回复类决定返回类型
    template<typename T, typename R>
    decltype(auto) request(const T& msg, R& result) {
        return request_impl(msg, result, priority_tag<2>{});
    }

    /// @brief 请求回复
    /// @tparam R 回复类型、或回复合并类类型
    /// @param result 回复、或回复合并类
    /// @return 回复,如果提供了合并类则由回复类决定返回类型
    template<typename R>
    decltype(auto) request(R& result) {
        return request_impl(result, priority_tag<2>{});
    }

    /// @brief 端口存根,用来绑定端口
    /// @tparam T 消息、请求参数类型
    /// @tparam R 回复类型
    template<typename T, typename R>
    struct port_stub {
        broker* owner{};

        /// @brief 绑定消息处理动作并返回存根
        /// @tparam Fn 消息处理动作类型
        /// @param fn 消息处理动作
        /// @return 消息处理动作存根
        template<typename Fn>
        action_stub_unit bind(Fn&& fn) {
            assert(owner != nullptr);
            auto handler = std::make_shared<message_handler<Fn, T, R>>(std::move(fn));
            owner->m_stubs.emplace_back(handler_stub{ type_code_of<T,R>(),handler });
            return[h = std::move(handler)](){};
        }
    };

    /// @brief 消息发布频道
    /// @tparam T 消息类型
    template<typename T>
    using channel_stub = port_stub<T, void>;

    /// @brief 请求回复端口
    /// @tparam T 请求参数类型
    /// @tparam R 回复类型
    template<typename T, typename R>
    using endpoint_stub = port_stub<T, R>;

    /// @brief 获取指定的消息发布频道
    /// @tparam T 消息类型
    /// @return 可订阅频道
    template<typename T>
    channel_stub<T> channel() noexcept {
        static_assert(!std::is_same_v<T, void>, "message type T cannot be void");
        return { this };
    }

    /// @brief 获取指定的请求回复端口
    /// @tparam T 请求参数类型,可为void
    /// @tparam R 回复类型
    /// @return 可请求回复端口
    template<typename T, typename R>
    endpoint_stub<T, R> endpoint() noexcept {
        static_assert(!std::is_same_v<R, void>, "response type  R cannot be void");
        return { this };
    }
public:
    /// @brief 消息处理动作存根
    class action_stub {
    public:
        action_stub() = default;
        action_stub(action_stub_unit&& stub)
        {
            m_actions.emplace_back(std::move(stub));
        }
        action_stub(std::vector<action_stub_unit>&& stubs)
            :m_actions(std::move(stubs)) {};

        action_stub(action_stub const&) = delete;
        action_stub& operator=(action_stub const&) = delete;

        action_stub(action_stub&& other) noexcept
            :m_actions(std::move(other.m_actions)) {};

        action_stub& operator=(action_stub&& other) noexcept {
            if (std::addressof(other) != this) {
                release();
                m_actions = std::move(other.m_actions);
            }
            return *this;
        }

        ~action_stub() noexcept {
            try { release(); }
            catch (...) {};
        }

        action_stub& operator+=(action_stub_unit&& stub) noexcept {
            m_actions.emplace_back(std::move(stub));
            return *this;
        }

        action_stub& operator+=(action_stub&& other) noexcept {
            if (std::addressof(other) != this) {
                m_actions.reserve(m_actions.size() + other.m_actions.size());
                for (auto& stub : other.m_actions) {
                    m_actions.emplace_back(std::move(stub));
                }
                other.m_actions.clear();
            }
            return *this;
        }

        /// @brief 释放消息处理动作
        void release() {
            for (auto& h : m_actions) {
                if (h) h();
            }
            m_actions.clear();
        }
    private:
        std::vector<action_stub_unit> m_actions;
    };
};


/// @brief 消息订阅辅助类
/// @tparam T 消息类型
/// @tparam E 使能
template<typename T, typename E = void>
struct subscribe_helper;

/// @brief 消息中间人的消息频道订阅辅助类实现
/// @tparam T 消息类型
template<typename T>
struct subscribe_helper<broker::channel_stub<T>> {
    template<typename Fn>
    static broker::action_stub_unit subscribe(broker::channel_stub<T> channel, Fn && fn) {
        return channel.bind(std::move(fn));
    }
};

/// @brief 消息订阅辅助类
/// @tparam T 消息处理类类型
template<typename T>
class subscriber {
    T* m_obj{};
    broker::action_stub m_stub;
public:
    explicit subscriber(T& obj) :m_obj(std::addressof(obj)) {};

    template<typename U, typename Fn>
    subscriber& subscribe(U& source, Fn&& fn) {
        m_stub += subscribe_helper<std::decay_t<U>>::subscribe(source,
            [obj = m_obj, h = std::move(fn)](auto arg){ std::invoke(h, obj, arg); }
        );
        return *this;
    }

    template<typename U>
    subscriber& subscribe(U& source) {
        m_stub += subscribe_helper<std::decay_t<U>>::subscribe(source,
            [obj = m_obj](auto& arg) { obj->on(arg); }
        );
        return *this;
    }

    template<typename U, typename Fn>
    subscriber& subscribe(broker& o, Fn&& fn) {
        return subscribe(o.channel<U>(), std::move(fn));
    }

    template<typename... Us>
    subscriber& subscribe(broker& o) {
        (subscribe(o.channel<Us>()), ...);
        return *this;
    }

    operator broker::action_stub() noexcept {
        return broker::action_stub{ std::move(m_stub) };
    }
};

template<typename T>
subscriber(T& obj)->subscriber<T>;

template<typename T, typename Fn>
broker::action_stub subscribe(T & source, Fn && fn) {
    return subscribe_helper<std::decay_t<T>>::subscribe(source, std::move(fn));
}

/// @brief 请求回复频道绑定辅助类
/// @tparam T 消息处理类类型
template<typename T>
class binder {
    T* m_obj{};
    broker::action_stub m_stub;
public:
    explicit binder(T& obj) :m_obj(std::addressof(obj)) {};

    /// @brief 使用lambda绑定消息中间人对应的请求回复端口
    template<typename U, typename R, typename Fn>
    binder& bind(broker::endpoint_stub<U, R>& ep, Fn&& fn) {
        if constexpr (std::is_same_v<U, void>) {
            m_stub += ep.bind(
                [obj = m_obj, h = std::move(fn)]
            (auto&& result){ std::invoke(h, obj, result); });
        }
        else
        {
            m_stub += ep.bind(
                [obj = m_obj, h = std::move(fn)]
            (auto&& arg, auto&& result){ std::invoke(h, obj, arg, result); });
        }
        return *this;
    }

    /// @brief 使用类的reply方法绑定到消息中间人对应的请求回复端口
    template<typename U, typename R>
    binder& bind(broker::endpoint_stub<U, R>& ep) {
        if constexpr (std::is_same_v<U, void>) {
            m_stub += ep.bind(
                [obj = m_obj](auto&& result) { obj->reply(result); }
            );
        }
        else
        {
            m_stub += ep.bind(
                [obj = m_obj](auto&& arg, auto&& result) { obj->reply(arg, result); }
            );
        }
        return *this;
    }

    /// @brief  使用lambda绑定消息中间人对应的请求回复端口
    template<typename U, typename R, typename Fn>
    binder& bind(broker& o, Fn&& fn) {
        return bind(o.endpoint<U, R>(), std::move(fn));
    }

    /// @brief 使用类的reply方法绑定到消息中间人对应的请求回复端口
    template<typename U, typename R>
    binder& bind(broker& o) {
        return bind(o.endpoint<U, R>());
    }

    /// @brief 转换为消息处理动作存根
    /// @return 
    operator broker::action_stub() noexcept {
        return broker::action_stub{ std::move(m_stub) };
    }
};

template<typename T>
binder(T& obj)->binder<T>;

template<typename T, typename R, typename Fn>
broker::action_stub bind(broker::endpoint_stub<T, R>& ep, Fn&& fn) {
    return ep.bind(std::move(fn));
}

/// @brief 回复辅助类,以支持多个回复
/// @tparam R 回复类型
template<typename R>
struct response {
    /// @brief 必须指定回复类型,否则无法识别
    using type = R;

    std::vector<R> results;

    /// @brief 提供alloc接口申请回复存储位置
    /// @return 
    R& alloc() {
        results.emplace_back(R{});
        return results.back();
    }

    /// @brief 返回回复收集的结果
    /// @return 
    std::vector<R> value() noexcept {
        return std::move(results);
    }
};
