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
