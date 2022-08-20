#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <type_traits>

class IValue {
public:
    virtual ~IValue() = default;
};

namespace impl {
    class MyObjectImpl :public IValue {
        int m_iV{};
        double m_dV{};
    public:
        int iV() const { return m_iV; }
        double dV() const { return m_dV; }
        void iV(int v) { m_iV = v; }
        void dV(double v) { m_dV = v; };
    };
}

template<typename T,typename E = void>
class Proxy;

//using MyObject = Proxy<impl::MyObjectImpl*>;

template<>
class Proxy<const impl::MyObjectImpl*> {
public:

};

template<>
class Proxy<impl::MyObjectImpl*> :public Proxy<const impl::MyObjectImpl*> {
public:
};


//要解决const和非const,必须有两套api实现,不可避免
//直接用底层类指针的方式不行,可以采用间接方式?
//譬如定义Proxy,由内部实现创建该对象?

//类如何暴露内部的容器?

//直接暴露:接口return?
//筛选等行为:实现XXXView
//变奏:自定义迭代器和View

// a>: 容器内容可直接暴露ContainerA -> ContainerA
// b>: 容器内容选择性暴露ContainerA -> ContainerA'
// c>: 容器内容需变化暴露ContainerA -> ContainerB

template<typename Container>
class ContainerView {
public:
    explicit ContainerView(Container& obj)
        :vp(std::addressof(obj)) {};

    ContainerView(const ContainerView& other) = delete;
    ContainerView operator=(const ContainerView& other) = delete;
    ContainerView(ContainerView&& other) noexcept = delete;
    ContainerView operator=(ContainerView&& other) noexcept = delete;

    auto begin() const noexcept { return vp->begin(); }
    auto end() const noexcept { return vp->end(); }
    auto begin() noexcept { return vp->begin(); }
    auto end() noexcept { return vp->end(); }

    auto operator->() const noexcept { return vp; }
    auto operator->() noexcept { return vp; }
private:
    Container* vp;
};

//使用Policy来进行过滤,变形
template<typename T>
class Policy{};


template<typename Container>
class ContainerFilterView {
    using ValueType = typename Container::iterator::value_type;
    using FilterType = std::function<bool(const ValueType&)>;
public:
    template<typename Iterator>
    struct FilterIterator : Iterator {
        FilterIterator() = default;
        FilterIterator(Iterator it, const ContainerFilterView& view)
            :Iterator(it), m_view(&view) {
            while (m_view->accept(*this)) {
                ++* this;
            }
        }

        FilterIterator& operator++() {
            do {
                Iterator::operator++();
            } while (m_view->accept(*this));
            return *this;
        }

        FilterIterator operator++(int) {
            FilterIterator result = *this;
            ++* this;
            return result;
        }
    private:
        const ContainerFilterView* m_view;
    };

    using const_iterator = FilterIterator<typename Container::const_iterator>;
    using iterator = FilterIterator<std::conditional_t<std::is_const_v<Container>, typename Container::const_iterator, typename Container::iterator>>;

    template<typename Fn>
    explicit ContainerFilterView(Container& vp, Fn&& fn)
        :m_vp(&vp), m_filter(fn) {};
    
    auto begin() const {
        return const_iterator{m_vp->begin(), *this};
    }

    auto end() const {
        return const_iterator{m_vp->end(), *this};
    }

    auto begin() {
        return iterator{m_vp->begin(), *this};
    }

    auto end() {
        return iterator{m_vp->end(), *this};
    }
private:
    template<typename Iterator>
    bool accept(const Iterator& it) const {
        return (it != m_vp->end() && !m_filter(*it));
    }
private:
    Container* m_vp;
    FilterType m_filter;
};

class Object {
public:
    auto begin() const { return m_values.begin(); }
    auto end() const { return m_values.end(); }
    auto begin() { return m_values.begin(); }
    auto end() { return m_values.end(); }

    auto  ValuesView() const noexcept {
        return ContainerView<const decltype(m_values)>{m_values};
    }

    auto  ValuesView() noexcept {
        return ContainerView<decltype(m_values)>{m_values};
    }

    const std::vector<std::string>&  View() const noexcept {
        return m_values;
    }

    std::vector<std::string>& View() noexcept {
        return m_values;
    }

    auto FilterView() const {
        return ContainerFilterView<const std::vector<std::string>>{
            m_values,
                [](auto& obj)->bool { return obj.size() < 4; }
        };
    }
    auto FilterView() {
        return ContainerFilterView<std::vector<std::string>>{
            m_values,
                [](auto& obj)->bool { return obj.size() < 4; }
        };
    }
private:
    std::vector<std::string> m_values;
};

int main() {
    Object obj{};

    auto& values = obj.View();
    values.emplace_back("0123");
    values.emplace_back("456");
    values.emplace_back("7890");
    values.emplace_back("a0");
    //for (auto& value : values) {
    //    value += ">";
    //}
    //for (auto&& value : obj.View()) {
    //    std::cout << value << "\n";
    //}
    for (auto&& value : obj.FilterView()) {
        std::cout << value << "\n";
    }
    return 0;
}
