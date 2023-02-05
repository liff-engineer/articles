// https://oliora.github.io/2015/12/29/pimpl-and-rule-of-zero.html
#pragma once

#include <type_traits>
#include <cassert>

namespace abc
{
    namespace detail
    {
        template<typename T>
        T* default_ctor(const T* src)
        {
            static_assert(sizeof(T) > 0, "default dtor can't delete incomplete type");
            static_assert(!std::is_void_v<T>, "default dtor can't delete incomplete type");
            return new T(*src);
        }

        template<typename T>
        void default_dtor(T*& ptr) noexcept {
            static_assert(sizeof(T) > 0, "default dtor can't delete incomplete type");
            static_assert(!std::is_void_v<T>, "default dtor can't delete incomplete type");
            delete ptr;
            ptr = nullptr;
        }
    }

    template<typename T>
    class ImplValue final {
    public:
        using dtor_t = void(*)(T*&);
        using ctor_t = T* (*)(const T*);

        struct Concept {
            ctor_t ctor;
            dtor_t dtor;

            static Concept Default() {
                Concept result{};
                result.ctor = detail::default_ctor<T>;
                result.dtor = detail::default_dtor<T>;
                return result;
            }
        };
    public:
        ImplValue() = default;
        ~ImplValue() {
            destruct();
        }

        explicit ImplValue(T* v, Concept vt = Concept::Default())
            :ImplValue(v, vt.ctor, vt.dtor) {};

        ImplValue(const ImplValue& other)
            :m_ptr{ other.construct() }, m_ctor{ other.m_ctor }, m_dtor{ other.m_dtor }
        {}

        ImplValue(ImplValue&& other) noexcept
            :m_ptr{ std::exchange(other.m_ptr,nullptr) },
            m_ctor{ std::exchange(other.m_ctor,nullptr) },
            m_dtor{ std::exchange(other.m_dtor,nullptr) }
        {}

        ImplValue& operator=(const ImplValue& other)
        {
            if (std::addressof(other) == this) return *this;
            destruct();
            m_ptr = other.construct();
            m_ctor = other.m_ctor;
            m_dtor = other.m_dtor;

            return *this;
        }

        ImplValue& operator=(ImplValue&& other) noexcept
        {
            if (std::addressof(other) == this) return *this;
            destruct();
            m_ptr = std::exchange(other.m_ptr, nullptr);
            m_ctor = std::exchange(other.m_ctor, nullptr);
            m_dtor = std::exchange(other.m_dtor, nullptr);
        }

        void swap(ImplValue& other) noexcept {
            using std::swap;
            swap(m_ptr, other.m_ptr);
            swap(m_ctor, other.m_ctor);
            swap(m_dtor, other.m_dtor);
        }

        explicit operator bool() const noexcept {
            return m_ptr != nullptr;
        }

        T* operator->() noexcept { return m_ptr; }
        const T* operator->() const noexcept { return m_ptr; }

        T* get() noexcept { return m_ptr; }
        const T* get() const noexcept { return m_ptr; }
    private:
        explicit ImplValue(T* ptr, ctor_t ctor, dtor_t dtor)
            :m_ptr{ ptr }, m_ctor{ ctor }, m_dtor{ dtor } {};

        void destruct() noexcept {
            if (!m_ptr || !m_dtor) return;
            m_dtor(m_ptr);
        }

        T* construct() const {
            if (m_ptr && m_ctor) return m_ctor(m_ptr);
            return nullptr;
        }
    private:
        T* m_ptr{};
        ctor_t m_ctor{};
        dtor_t m_dtor{};
    };

    template<typename T, typename... Args>
    inline ImplValue<T> MakeImplValue(Args&&... args) {
        return ImplValue<T>{
            new T(std::forward<Args>(args)...)
        };
    }

    template<typename T, typename... Args>
    inline ImplValue<T> MakeImplValue(typename ImplValue<T>::Concept vt, Args&&... args) {
        return ImplValue<T>{
            new T(std::forward<Args>(args)...),
                vt
        };
    }

    template<typename T>
    inline void swap(ImplValue<T>& lhs, ImplValue<T>& rhs) noexcept {
        return lhs.swap(rhs);
    }
}

namespace std
{
    template<typename T>
    struct hash<::abc::ImplValue<T>>
    {
        std::size_t operator()(const ::abc::ImplValue<T>& v) const noexcept {
            return hash<const T*>()(v.get());
        }
    };
}
