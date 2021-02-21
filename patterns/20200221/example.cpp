#include <tuple>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <initializer_list>
#include <type_traits>

class IArgument
{
public:
    virtual ~IArgument() = default;

    template<typename T>
    bool is() const noexcept;
    
    template<typename T>
    T& as() & noexcept;
};

template<typename T>
class Argument final:public IArgument
{
public:
    T v;
    Argument(T v_arg)
        :v(std::move(v_arg)) {};
};

template<typename T>
bool IArgument::is() const noexcept{
    return dynamic_cast<const Argument<T>*>(this) != nullptr;
}

template<typename T>
T& IArgument::as() & noexcept {
    return static_cast<Argument<T>*>(this)->v;
}


template<typename... Ts>
class Arguments {
    std::tuple<std::optional<Argument<Ts>>...> m_values;
    std::array<IArgument*, sizeof...(Ts)> m_pointers{};
public:
    Arguments() {
        m_pointers.fill(nullptr);
    };

    template<std::size_t I,typename T>
    void set(T v)
    {
        if (IArgument* arg = std::get<I>(m_pointers)) {
            if (!arg->is<T>()) {
                throw std::invalid_argument("invalid setting");
            }
            arg->as<T>() = v;
        }
        else
        {
            //更新值并刷新指针
            std::get<I>(m_values) = v;
            std::get<I>(m_pointers) = std::get<I>(m_values).operator->();
        }
    }

    template<std::size_t I>
    void reset()
    {
        std::get<I>(m_pointers) = nullptr;
        std::get<I>(m_values).reset();
    }
    
    template<std::size_t I>
    IArgument* at() noexcept {
        return std::get<I>(m_pointers);
    }

    IArgument* at(std::size_t i) {
        return m_pointers.at(i);
    }

    template<std::size_t ...Is>
    std::tuple<Ts...> asImpl(std::index_sequence<Is...> is) const
    {
        return std::make_tuple(std::get<Is>(m_values).value().v...);
    }

    decltype(auto) as() const {
        return asImpl(std::make_index_sequence<sizeof...(Ts)>{});
    }

    template<std::size_t I>
    void refresh() noexcept {
        if (std::get<I>(m_values)) {
            std::get<I>(m_pointers) = std::get<I>(m_values).operator->();
        }
    }

    template<std::size_t ...Is>
    void refreshImpl(std::index_sequence<Is...> is) noexcept
    {
        (refresh<Is>(), ...);
    }

    void refresh()
    {
        refreshImpl(std::make_index_sequence<sizeof...(Ts)>{});
    }
};

class ICallback
{
public:
    virtual ~ICallback() = default;

    virtual void execute() = 0;
    virtual IArgument* argument(std::string_view name) noexcept = 0;

    template<typename T>
    void setArgument(std::string_view name, T v) {
        auto arg = argument(name);
        if (arg && arg->is<T>()) {
            arg->as<T>() = v;
        }
    }

    template<typename T>
    std::optional<T> argumentAs(std::string_view name) noexcept {
        auto arg = argument(name);
        if (arg && arg->is<T>()) {
            return arg->as<T>();
        }
        return {};
    }
};

template<typename ... Ts>
class Callback:public ICallback
{
    std::array<std::string_view, sizeof...(Ts)> m_names;
    void(*m_addr)(Ts...) = nullptr;
public:
    Arguments<Ts...> args;
public:
    Callback() = default;
    Callback(void(*f)(Ts...), std::array<std::string_view, sizeof...(Ts)> names)
        :m_addr(f), m_names(names) {};

    explicit operator bool() const noexcept {
        return m_addr != nullptr;
    }

    void operator()() {
        std::apply(m_addr, args.as());
    }

    void execute()  override {
        std::apply(m_addr, args.as());
    }

    IArgument* argument(std::string_view name) noexcept override {
        for (std::size_t i = 0; i < m_names.size(); i++) {
            if (m_names[i] == name) {
                return args.at(i);
            }
        }
        return nullptr;
    }
};

#include <iostream>


void callback_example(ICallback& cb)
{
    //查询参数
    auto iV = cb.argumentAs<int>("iV");
    auto dV = cb.argumentAs<double>("dV");
    auto sV = cb.argumentAs<std::string>("sV");
    if (iV) {
        std::cout << "iV:" << iV.value() << "\n";
    }
    if (dV) {
        std::cout << "dV:" << dV.value() << "\n";
    }
    if (sV) {
        std::cout << "sV:" << sV.value() << "\n";
    }

    //执行
    cb.execute();

    //修改参数
    cb.setArgument("iV", 123456);
    cb.setArgument("dV", 1.717);
    cb.setArgument("sV", std::string{"Garfield"});
    //执行
    cb.execute();
}

int main(int argc, char** agrv) {
    Arguments<int, double, std::string> args;
    args.set<0, int>(1024);
    args.set<1, double>(3.1415926);
    args.set<2, std::string>("liff.engineer@gmail.com");

    args.reset<0>();

    auto v = args.at(1)->as<double>();
    args.set<0, int>(8192);

    auto t = args.as();

    Callback<int, double, std::string> cb([](int iV,double dV,std::string sV) {
        std::cout << "iV:" << iV << "\ndV:" << dV << "\nsV:" << sV << "\n";
        },
        {"iV","dV","sV"});

    cb.args = args;
    cb.args.refresh();
    cb();
    cb.args.set<1, double>(1.414);
    cb();

    callback_example(cb);
    
    return 0;
}
