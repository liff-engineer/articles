#pragma once
//TODO inject函数可以接收类型成员函数指针,以此来支持用户自由定义成员
#include <type_traits>
#include <memory>
#include <vector>

namespace abc
{
    /// @brief 编译期字符串哈希值计算
    namespace TypeCodeImpl {
        template <typename T>
        struct fnv1a_constant;

        template <>
        struct fnv1a_constant<std::uint32_t>
        {
            static constexpr std::uint32_t prime = 16777619;
            static constexpr std::uint32_t offset = 2166136261;
        };

        template <>
        struct fnv1a_constant<std::uint64_t>
        {
            static constexpr std::uint64_t prime = 1099511628211ull;
            static constexpr std::uint64_t offset = 14695981039346656037ull;
        };

        template <typename T>
        inline constexpr T hash_fnv1a(std::size_t n, 
            const char* const str, const T result = fnv1a_constant<T>::offset)
        {
            return n > 0 ? hash_fnv1a(n - 1, str + 1, 
                (result ^ *str) * fnv1a_constant<T>::prime) : result;
        }

        template <typename T, std::size_t N>
        inline constexpr T hash(const char(&str)[N])
        {
            return hash_fnv1a<T>(N - 1, &str[0]);
        }

        template <typename T>
        inline constexpr T hash(const char* const str, std::size_t n)
        {
            return hash_fnv1a<T>(n, str);
        }
    }

    /// @brief 类型标识符:编译期字符串常量,包含哈希值以支持快速比较
    struct TypeCode {
        const char* literal{ nullptr };
        std::size_t size{ 0 };
        std::size_t hash{ 0 };

        TypeCode() = default;

        template<std::size_t N>
        constexpr TypeCode(const char(&buf)[N])
            :literal(buf), size(N - 1), hash(TypeCodeImpl::hash<std::size_t>(buf))
        {}

        constexpr TypeCode(const char* const buf, std::size_t n)
            : literal(buf), size(n), hash(TypeCodeImpl::hash<std::size_t>(buf, n))
        {}
    };

    constexpr bool operator==(TypeCode const& lhs, TypeCode const& rhs) noexcept {
        return lhs.hash == rhs.hash;
    }

    constexpr bool operator!=(TypeCode const& lhs, TypeCode const& rhs) noexcept {
        return !(lhs == rhs);
    }

    constexpr bool operator<(TypeCode const& lhs, TypeCode const& rhs) noexcept {
        return lhs.hash < rhs.hash;
    }

    constexpr bool operator<=(TypeCode const& lhs, TypeCode const& rhs) noexcept {
        return lhs.hash <= rhs.hash;
    }

    constexpr bool operator>(TypeCode const& lhs, TypeCode const& rhs) noexcept {
        return lhs.hash > rhs.hash;
    }

    constexpr bool operator>=(TypeCode const& lhs, TypeCode const& rhs) noexcept {
        return lhs.hash >= rhs.hash;
    }

    /// @brief 利用函数名宏为类型列表提供标识符
    /// @tparam ...Ts 类型列表
    /// @return 类型标识符
    template<typename... Ts>
    constexpr TypeCode Code() noexcept {
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

    /// @brief 指定类型序列的标识符
    /// @tparam ...Ts 类型序列
    template<typename... Ts>
    struct TypeCodeInject {
        static constexpr auto value = Code<Ts...>();
    };

    /// @brief 类型序列标识符获取
    template<typename... Ts>
    constexpr TypeCode TypeCodeOf()  noexcept {
        return TypeCodeInject<Ts...>::value;
    }

    inline namespace v1
    {
        namespace detail {
            template<typename T>
            const void* addressOfImpl(const T& obj, std::true_type) {
                return dynamic_cast<const void*>(&obj);
            }

            template<typename T>
            const void* addressOfImpl(const T& obj, std::false_type) {
                return &obj;
            }

            template<typename T>
            const void* addressOf(const T& obj) {
                return addressOfImpl(obj,
                    std::conditional_t<!std::is_polymorphic_v<T> ||
                    std::is_final_v<T>, std::false_type, std::true_type>{});
            }
        }

        /// @brief 安全地将目标类型转换为const void*
        /// @tparam T 目标类型
        /// @param obj 类实例
        /// @return 
        template<typename T>
        const void* addressOf(const T& obj) {
            return detail::addressOf(obj);
        }

        /// @brief 响应函数负载:用来存储参数地址、结果地址
        struct Payload {
            const void* arg{ nullptr };
            void* result{ nullptr };
            TypeCode    code{ TypeCodeOf<void,void>() };

            Payload() = default;

            template<typename Arg, typename R>
            explicit Payload(const Arg& arg, R& r)
                :arg(addressOf(arg)),
                result(const_cast<void*>(addressOf(r))),
                code(TypeCodeOf<Arg, R>()) {};

            template<typename Arg>
            explicit Payload(const Arg& arg)
                :arg(addressOf(arg)),
                result(nullptr),
                code(TypeCodeOf<Arg>()) {};
        };

        /// @brief 响应函数:对应动作函数;只支持单个参数及可选的单个返回值
        class IHandler {
        public:
            virtual ~IHandler() = default;

            template<typename Arg, typename R>
            void run(const Arg& arg, R& r) {
                return runImpl(Payload(arg, r));
            }

            template<typename Arg>
            void run(const Arg& arg) {
                return runImpl(Payload(arg));
            }
        protected:
            virtual void runImpl(Payload payload) = 0;
        };

        /// @brief 基于CRTP的响应函数辅助实现,可以基于目标类型注册多种响应函数实现
        /// @tparam T 派生类类型
        template<typename T>
        class Handler :public IHandler
        {
        protected:
            struct Callback {
                TypeCode code;
                void(*cb)(T*, Payload);
            };
            std::vector<Callback> m_callbacks;

            void runImpl(Payload payload) override {
                for (auto& obj : m_callbacks) {
                    if (obj.code != payload.code)
                        continue;
                    obj.cb(static_cast<T*>(this), payload);
                    return;
                }
            }
        };

        /// @brief 仿函数版本的响应函数,使用inject可以注册多个不同参数的响应函数
        /// @tparam T 仿函数类型
        template<typename T>
        class CallableHandler final :public Handler<CallableHandler<T>>
        {
            T m_obj;
        public:
            explicit CallableHandler(T v)
                :m_obj(std::move(v)) {};

            /// @brief 带结果写入的响应函数注入
            /// @tparam Arg 响应函数的参数
            /// @tparam R 响应函数执行的结果
            /// @return 
            template<typename Arg, typename R>
            CallableHandler& inject() {
                m_callbacks.emplace_back(Callback{ TypeCodeOf<Arg,R>(),
                    [](CallableHandler* obj,Payload o) {
                        obj->m_obj(*static_cast<const Arg*>(o.arg),
                            *static_cast<R*>(o.result));
                    } });
                return *this;
            }

            template<typename Arg>
            CallableHandler& inject() {
                m_callbacks.emplace_back(Callback{ TypeCodeOf<Arg>(),
                    [](CallableHandler* obj,Payload o) {
                        obj->m_obj(*static_cast<const Arg*>(o.arg));
                    } });
                return *this;
            }
        };

        /// @brief 类版本的响应函数,使用inject可以注册多个不同参数的响应函数,要求类中响应函数定义必须名称为on
        /// @tparam T 分发类的类型
        template<typename T>
        class DispatchHandler final :public Handler<DispatchHandler<T>>
        {
            T m_obj;
        public:
            explicit DispatchHandler(T v)
                :m_obj(std::move(v)) {};

            /// @brief 带结果写入的响应函数注入
            /// @tparam Arg 响应函数的参数
            /// @tparam R 响应函数执行的结果
            /// @return 
            template<typename Arg, typename R>
            DispatchHandler& inject() {
                m_callbacks.emplace_back(Callback{ TypeCodeOf<Arg,R>(),
                    [](DispatchHandler* obj,Payload o) {
                        obj->m_obj.on(*static_cast<const Arg*>(o.arg),*static_cast<R*>(o.result));
                    } });
                return *this;
            }

            template<typename Arg>
            DispatchHandler& inject() {
                m_callbacks.emplace_back(Callback{ TypeCodeOf<Arg>(),
                    [](DispatchHandler* obj,Payload o) {
                        obj->m_obj.on(*static_cast<const Arg*>(o.arg));
                    } });
                return *this;
            }
        };

        /// @brief 创建仿函数版本的响应函数
        /// @tparam F 仿函数类型
        /// @param fn 仿函数类实例
        /// @return 
        template<typename F>
        auto MakeCallableHandler(F&& fn) noexcept {
            return CallableHandler<std::decay_t<F>>(std::forward<F>(fn));
        }

        /// @brief 创建类版本的响应函数
        /// @tparam T 类类型
        /// @param v 类实例
        /// @return 
        template<typename T>
        auto MakeDispatchHandler(T&& v) noexcept {
            return DispatchHandler<std::decay_t<T>>(std::forward<T>(v));
        }

        /// @brief 数据写入接口:用来支持动作序列化
        struct IWriter {
            virtual ~IWriter() = default;

            enum class When {
                enter, //动作开始处理前
                leave, //动作处理完成后
            };

            virtual void push(Payload const& o,When when) = 0;
        };

        /// @brief 动作:存储参数,结果地址等信息
        struct IAction {
            virtual ~IAction() = default;

            /// @brief 利用动作触发执行响应函数
            /// @param op 响应函数
            virtual void exec(IHandler& op) = 0;

            virtual void write(IWriter& writer, IWriter::When when) const = 0;
        };

        /// @brief 动作实现:包含不变参数地址、可变结果地址
        /// @tparam Result 结果类型
        /// @tparam Arg 参数类型
        template<typename Arg, typename Result>
        struct Action final : IAction {
            const Arg* arg;
            Result* r;
            explicit Action(const Arg& arg, Result& result)
                :arg(std::addressof(arg)), r(std::addressof(result)) {};
            void exec(IHandler& op) override {
                op.run(*arg, *r);
            }
            void write(IWriter& writer, IWriter::When when) const override {
                writer.push(Payload(*arg, *r),when);
            }
        };

        /// @brief 动作实现:没有结果的偏特化版本
        /// @tparam Arg 参数类型
        template<typename Arg>
        struct Action<Arg, void> final :IAction {
            const Arg* arg;
            explicit Action(const Arg& arg)
                :arg(std::addressof(arg)) {};
            void exec(IHandler& op) override {
                op.run(*arg);
            }
            void write(IWriter& writer, IWriter::When when) const override {
                writer.push(Payload(*arg),when);
            }
        };

        /// @brief 支持回放的动作
        struct IReplayAction : IAction {

            /// @brief 验证回放结果与预期结果是否一致
            /// @return 验证结果
            virtual bool verify() const = 0;
        };

        /// @brief 回放动作实现:包含参数、预期结果、结果
        /// @tparam Arg 参数类型
        /// @tparam Result 结果类型
        template<typename Arg, typename Result>
        struct ReplayAction final :IReplayAction {
            Arg arg{};
            Result expect{};
            Result actual{};

            void exec(IHandler& op) override {
                op.run(arg, actual);
            }
            void write(IWriter& writer, IWriter::When when) const override {
                writer.push(Payload(arg,actual),when);
            }

            bool verify() const override {
                return expect == actual;
            }
        };

        /// @brief 回放动作实现:无结果的偏特化版本
        /// @tparam Arg 参数类型
        template<typename Arg>
        struct ReplayAction<Arg, void> final :IReplayAction {
            Arg arg{};
            void exec(IHandler& op) override {
                op.run(arg);
            }
            void write(IWriter& writer, IWriter::When when) const override {
                writer.push(Payload(arg),when);
            }

            bool verify() const override { return true; }
        };

        /// @brief 动作分发器
        class Broker final
        {
        public:
            Broker() = default;

            /// @brief 指定动作记录模块的构造函数
            /// @param writer 动作写入接口
            explicit Broker(std::shared_ptr<IWriter> writer)
                :m_writer(writer) {};

            /// @brief 设置动作记录模块
            /// @param writer 动作写入接口 
            void setWriter(std::shared_ptr<IWriter> writer) {
                m_writer = writer;
            }

            /// @brief 注册动作响应函数
            /// @param handler 响应函数
            /// @return 响应函数副本
            std::shared_ptr<IHandler> registerHandler(std::shared_ptr<IHandler>&& handler) noexcept {
                m_handlers.emplace_back(std::move(handler));
                return m_handlers.back();
            }

            /// @brief 以类/仿函数响应函数实例来进行注册
            /// @tparam T 类/仿函数响应函数
            /// @param v 实例
            /// @return 响应函数副本
            template<typename T>
            std::shared_ptr<IHandler> registerHandler(T&& v) noexcept {
                static_assert(std::is_base_of<IHandler, std::decay_t<T>>::value, "should be base of IHandler");
                m_handlers.emplace_back(std::make_shared<std::decay_t<T>>(std::forward<T>(v)));
                return m_handlers.back();
            }

            /// @brief 分发特定参数及结果类型的动作
            /// @tparam Arg 参数类型
            /// @tparam Result 结果类型
            /// @param arg 参数
            /// @param r 结果
            template<typename Arg, typename Result>
            void dispatch(const Arg& arg, Result& r) {
                dispatchImpl(Action<Arg, Result>{arg, r});
            }

            template<typename Result, typename Arg>
            Result dispatch(const Arg& arg) {
                Result r{};
                dispatch(arg, r);
                return r;
            }

            /// @brief 分发特定参数类型的动作
            /// @tparam Arg 参数类型
            /// @param arg 参数
            template<typename Arg>
            void dispatch(const Arg& arg) {
                dispatchImpl(Action<Arg, void>{arg});
            }

            /// @brief 回放动作
            /// @param msg 
            void replay(IAction& action) {
                auto handle = [&](auto& action) {
                    for (auto& h : m_handlers) {
                        action.exec(*h);
                    }
                };
                if (m_writer) {
                    action.write(*m_writer, IWriter::When::enter);
                    handle(action);
                    action.write(*m_writer, IWriter::When::leave);
                }
                else {
                    handle(action);
                }
            }
        private:
            template<typename Arg, typename R>
            void dispatchImpl(Action<Arg, R>& action) {
                auto handle = [&](auto& action) {
                    for (auto& h : m_handlers) {
                        action.exec(*h);
                    }
                };
                if (m_writer) {
                    action.write(*m_writer, IWriter::When::enter);
                    handle(action);
                    action.write(*m_writer, IWriter::When::leave);
                }
                else {
                    handle(action);
                }
            }
            std::vector<std::shared_ptr<IHandler>> m_handlers;
            std::shared_ptr<IWriter>  m_writer;
        };
    }
}

namespace std
{
    template<>
    struct hash<abc::TypeCode>
    {
        std::size_t operator()(abc::TypeCode const& v) const noexcept {
            return v.hash;
        }
    };
}
