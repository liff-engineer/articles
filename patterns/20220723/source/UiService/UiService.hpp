#pragma once
#include "AnyVector.hpp"

class IInputer {
public:
    virtual ~IInputer() = default;
    virtual void stop() {};
};

class UiService {
public:
    virtual ~UiService() = default;

    virtual void stop();

    template<typename T, typename... Args>
    T* install(Args&&... args) {
        auto service = m_channels.emplace<T>(std::forward<Args>(args)...);
        if constexpr (std::is_base_of_v<IInputer, T>) {
            m_channels.emplace<IInputer>(service);
        }
        return service;
    }

    template<typename I, typename T>
    I* bind(T* v) {
        static_assert(std::is_base_of_v<I, T>, "std::is_base_of_v<I, T>");
        return m_channels.emplace<I>(v);
    }
protected:
    AnyVector m_channels;
};
