#pragma once

#include "AnyVector.hpp"

class UiContext final {
public:
    template<typename I, typename T = I>
    void attach(T* obj) {
        static_assert(std::is_base_of_v<I, T>, "std::is_base_of_v<I, T>");
        m_targets.emplace<I>(obj);
    }
    
    template<typename T>
    void detach(T* obj) {
        m_targets.erase(obj);
    }

    template<typename T>
    T* find() const noexcept {
        return m_targets.find<T>();
    }
private:
    AnyVector m_targets;
};
