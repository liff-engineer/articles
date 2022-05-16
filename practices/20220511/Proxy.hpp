#pragma once
#include <type_traits>

namespace abc
{
    /// @brief 类型T的代理类
    /// @tparam T 指针类型或者可以转换为bool的类型
    template<typename T>
    class Proxy {
        static_assert(std::is_pointer<T>::value
            || (std::is_constructible<bool, T>::value
                && !std::is_convertible<T, bool>::value
                ), "T should be pointer type or has `explicit operator bool()`!");
    public:
        Proxy() = default;
        explicit Proxy(T obj) :m_obj(std::move(obj)) {};

        Proxy(const Proxy& other) = default;
        Proxy& operator=(const Proxy& other) = default;
        Proxy(Proxy&& other) noexcept = default;
        Proxy& operator=(Proxy&& other) noexcept = default;

        virtual ~Proxy() = default;

        auto operator->() noexcept { return GetPointer(this); }
        auto operator->() const noexcept { return GetPointer(this); }

        decltype(auto) Get() noexcept { return GetImpl(this); }
        decltype(auto) Get() const noexcept { return GetImpl(this); }

        explicit operator bool() const noexcept {
            if (!m_obj) return false;
            return IsValidImpl();
        }
    protected:
        using Super = Proxy<T>;
        virtual bool IsValidImpl() const noexcept {
            return true;
        }
        T m_obj{};
    private:
        //保证获取正确的类型;
        //技术细节参见C++标准提案P0847(Deducing this)的"Motivation"章节
        template<typename U>
        static decltype(auto) GetImpl(U&& vp) {
            return vp->m_obj;
        }

        template<typename U>
        static decltype(auto) GetPointer(U&& vp) {
            return GetPointerImpl(std::forward<U>(vp), std::is_pointer<T>{});
        }

        template<typename U>
        static decltype(auto) GetPointerImpl(U&& vp, std::true_type) {
            return vp->m_obj;
        }

        template<typename U>
        static decltype(auto) GetPointerImpl(U&& vp, std::false_type) {
            return std::addressof(vp->m_obj);
        }
    };
}
