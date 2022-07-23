#pragma once

#include <type_traits>
#include <utility>
#include <iostream>

namespace abc
{
    template<typename T>
    decltype(auto) chain(T&& obj) {
        return std::forward<T>(obj);
    }

    template<typename U, typename... Args, typename T>
    decltype(auto) chain(T&& obj, U&& op, Args&&... args) {
        //static_assert(std::is_convertible_v<T, bool>, "std::is_convertible_v<T, bool>");
        if constexpr (std::is_convertible_v<T, bool>) {
            std::cout << typeid(T).name() << " :can be test!\n";
        }
        return chain(op.op(std::forward<T>(obj)), std::forward<Args>(args)...);
    }

    template<typename R>
    struct cast {
        template<typename S>
        R* op(S* obj) {
            static_assert(std::is_base_of_v<S, R>, "std::is_base_of_v<S,R>");
            return dynamic_cast<R*>(obj);
        }

        template<typename S>
        const R* op(const S* obj) {
            static_assert(std::is_base_of_v<S, R>, "std::is_base_of_v<S,R>");
            return dynamic_cast<const R*>(obj);
        }
    };
}

namespace easy
{
    /// @brief 从类型T读取类型R
    template<typename T, typename R, typename E = void>
    struct getter;

    /// @brief 针对派生类情况的处理
    template<typename T, typename R>
    struct getter<T, R, std::enable_if_t<std::is_base_of<T, R>::value>> {
        R* get(T* o) {
            return dynamic_cast<R*>(o);
        }

        const R* get(const T* o) {
            return dynamic_cast<const R*>(o);
        }
    };

    namespace detail
    {
        template<typename R, typename T>
        decltype(auto) get_impl(T* o) {
            return getter<T, R>{}.get(o);
        }

        template<typename R, typename T>
        decltype(auto) get_impl(const T* o) {
            return getter<T, R>{}.get(o);
        }
    }

    /// @brief 终止条件
    template<typename T>
    decltype(auto) get(T&& obj) {
        return std::forward<T>(obj);
    }

    /// @brief 链式Get
    template<typename U, typename ...Ts, typename T>
    decltype(auto) get(T&& obj) {
        return get<Ts...>(detail::get_impl<U>(std::forward<T>(obj)));
    }

}
