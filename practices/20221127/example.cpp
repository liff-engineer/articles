#include <iostream>
#include <memory>
#include <vector>
#include <string>

class IValue
{
public:
    struct Tag {};

    virtual ~IValue() = default;
    virtual void print() const noexcept = 0;
};

template<typename... Args>
std::unique_ptr<IValue> CreateValue(Args&&... args) { 
    static_assert(false, "Need define Your CreateValue implement");
    return nullptr;
}

class Value {
    std::unique_ptr<IValue> m_v;
public:
    template<typename... Args>
    Value(Args&&... args)
        :m_v(CreateValue(IValue::Tag{}, std::forward<Args>(args)...))
    {};

    explicit operator bool() const noexcept {
        return m_v.operator bool();
    }

    operator std::unique_ptr<IValue>() && noexcept {
        return std::move(m_v);
    }

    decltype(auto) operator->() const noexcept { return m_v.get(); }
    decltype(auto) operator->() noexcept { return m_v.get(); }
};

class Int :public IValue {
    int m_v;
public:
    explicit Int(int v) :m_v(v) {};
    void print() const noexcept {
        std::cout << __FUNCSIG__ << ":" << m_v << "\n";
    }
};

class Bool :public IValue {
    bool m_v;
public:
    explicit Bool(bool v) :m_v(v) {};
    void print() const noexcept {
        std::cout << __FUNCSIG__ << ":" << m_v << "\n";
    }
};

class Double :public IValue {
    double m_v;
public:
    explicit Double(double v) :m_v(v) {};
    void print() const noexcept {
        std::cout << __FUNCSIG__ << ":" << m_v << "\n";
    }
};


class String :public IValue {
    std::string m_v;
public:
    explicit String(std::string&& v) :m_v(std::move(v)) {};
    void print() const noexcept {
        std::cout << __FUNCSIG__ << ":" << m_v << "\n";
    }
};

std::unique_ptr<IValue> CreateValue(IValue::Tag, int v) {
    return std::make_unique<Int>(v);
}

std::unique_ptr<IValue> CreateValue(IValue::Tag, bool v) {
    return std::make_unique<Bool>(v);
}

std::unique_ptr<IValue> CreateValue(IValue::Tag, double v) {
    return std::make_unique<Double>(v);
}

std::unique_ptr<IValue> CreateValue(IValue::Tag, std::string&& v) {
    return std::make_unique<String>(std::move(v));
}

//注意,传递字符串常量时不会自动进入std::string条件,因而需要专门写
template<std::size_t N>
std::unique_ptr<IValue> CreateValue(IValue::Tag, const char(&str)[N]) {
    return std::make_unique<String>(std::string{str});
}

int main() {
    auto bV = Value{ true };
    auto iV = Value{ 1024 };
    auto dV = Value{ 1.414 };
    //auto sV = Value{ std::string{"liff.engineer@gmail.com"} };
    auto sV = Value{ "liff.engineer@gmail.com" };

    //注意初始化列表不支持无法copy的对象
    //std::vector<Value> v0s{
    //    Value{true},
    //    Value{1024},
    //    Value{1.414},
    //    Value{"liff.engineer@gmail.com" }
    //};

    std::vector<Value> v1s{};
    v1s.emplace_back(true);
    v1s.emplace_back(1024);
    v1s.emplace_back(1.414);
    v1s.emplace_back(std::string{ "liff.engineer@gmail.com" });
    for (auto&& v : v1s) {
        v->print();
    }

    std::vector<std::unique_ptr<IValue>> v2s{};
    v2s.emplace_back(Value{ true });
    v2s.emplace_back(Value{ 1024 });
    v2s.emplace_back(Value{ 1.414 });
    v2s.emplace_back(Value{ std::string{ "liff.engineer@gmail.com" } });
    for (auto&& v : v2s) {
        v->print();
    }
    return 0;
}
