#pragma once

#include <memory>
#include <vector>
#include <string>

class AnyVector {
public:
    template<typename T, typename... Args>
    T* emplace(Args&&... args) {
        auto v = std::make_unique<Value<T>>(m_values.size(), std::forward<Args>(args)...);
        auto result = v->get();
        m_values.emplace_back(std::move(v));
        return result;
    }

    template<typename I, typename T>
    I* emplace(T* obj) {
        if constexpr (std::is_base_of_v<I, T>) {
            return emplace_pointer<I>(obj);
        }
        else
        {
            auto v = std::make_unique<Value<I>>(m_values.size(), obj);
            auto result = v->get();
            m_values.emplace_back(std::move(v));
            return result;
        }
    }

    template<typename T>
    std::size_t erase(T* obj) {
        auto code = typeid(T).name();
        std::size_t result{};
        for (auto& v : m_values) {
            if (!v || std::strcmp(v->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Proxy<T>*>(v.get())) {
                if (vp->get() == obj) {
                    result += erase_impl(v->index());
                }
            }
        }
        return result;
    }

    template<typename T>
    std::size_t erase() {
        auto code = typeid(T).name();
        std::size_t result{};
        for (auto& v : m_values) {
            if (!v || std::strcmp(v->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Proxy<T>*>(v.get())) {
                result += erase_impl(v->index());
            }
        }
        return result;
    }

    template<typename T>
    T* find() const noexcept {
        auto code = typeid(T).name();
        for (auto& v : m_values) {
            if (!v || std::strcmp(v->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Proxy<T>*>(v.get())) {
                return vp->get();
            }
        }
        return nullptr;
    }

    template<typename T, typename Fn>
    void visit(Fn&& fn) const noexcept {
        auto code = typeid(T).name();
        for (auto& v : m_values) {
            if (!v || std::strcmp(v->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Proxy<T>*>(v.get())) {
                fn(vp->get());
            }
        }
    }
private:
    class IValue {
    public:
        virtual ~IValue() = default;
        virtual const char* code() const noexcept = 0;
        virtual std::size_t index() const noexcept = 0;
    };

    template<typename I>
    class Proxy :public IValue {
    public:
        Proxy() = default;
        template<typename T>
        Proxy(T* v, std::size_t i)
            :m_vp(v), m_index(i)
        {}

        const char* code() const noexcept override {
            return typeid(I).name();
        }

        std::size_t index() const noexcept final { return m_index; }
        I* get() const noexcept { return m_vp; }
    protected:
        I* m_vp{};
        std::size_t m_index;
    };

    template<typename T>
    class Value final :public Proxy<T> {
    public:
        template<typename... Args>
        explicit Value(std::size_t i, Args&&... args)
            :m_v(std::forward<Args>(args)...)
        {
            m_vp = &m_v;
            m_index = i;
        }

        const char* code() const noexcept override {
            return typeid(T).name();
        }
    private:
        T  m_v;
    };
private:
    template<typename I, typename T>
    I* emplace_pointer(T* obj) {
        auto code = typeid(T).name();

        bool exist = false;
        auto index = m_values.size();
        for (auto& v : m_values) {
            if (!v || std::strcmp(v->code(), code) != 0)
                continue;
            if (auto vp = dynamic_cast<Proxy<T>*>(v.get())) {
                if (vp->get() == obj) {
                    index = v->index();
                    exist = true;
                }
            }
        }

        if (!exist) {
            m_values.emplace_back(std::make_unique<Proxy<T>>(obj, index));
        }

        if constexpr (!std::is_same_v<I, T>) {
            m_values.emplace_back(std::make_unique<Proxy<I>>(obj, index));
        }
        return obj;
    }
    inline std::size_t erase_impl(std::size_t i) {
        std::size_t result{};
        for (auto& v : m_values) {
            if (v && v->index() == i) {
                v = nullptr;
                result += 1;
            }
        }
        return result;
    }
private:
    std::vector<std::unique_ptr<IValue>> m_values;
};
