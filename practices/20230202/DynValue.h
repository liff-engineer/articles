#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <cassert>

namespace abc
{
    //要求:
    // 1. clone - 复制
    // 2. equal - ==比较
    namespace detail
    {
        template<typename T, typename U>
        std::unique_ptr<T> to(std::unique_ptr<U>&& v) {
            if constexpr (std::is_base_of_v<T, U>) {
                return std::move(v);
            }
            else
            {
                return std::unique_ptr<T>(static_cast<T*>(v.release()));
            }
        }

        template<typename T>
        std::unique_ptr<T> clone(const T& v) {
            return to<T>(v.clone());
        }
    }

    template<typename T>
    class DynValue;

    template<typename T>
    struct IsDynValue :std::false_type {};

    template<typename T>
    struct IsDynValue<DynValue<T>> :std::true_type {};

    template<typename T>
    class DynValue final {
        //只接受多态类型
        static_assert(std::is_polymorphic_v<T>, "only accept polymorphic type");

        template <class U>
        friend class DynValue;

        std::unique_ptr<T> m_ptr{};
    public:
        ~DynValue() = default;

        DynValue() = default;

        template<typename U>
        explicit DynValue(std::unique_ptr<U> v)
            :m_ptr{ std::move(v) } {};

        template<typename U, typename = std::enable_if_t<std::is_base_of_v<T, U>>>
        explicit DynValue(const U* u)
        {
            if (!u) return;
            const T* v = u;
            m_ptr = detail::clone<T>(*v);
        }

        template<typename U, typename = std::enable_if_t<std::is_base_of_v<T, U> && !std::is_same_v<T, U>>>
        explicit DynValue(DynValue<U> v)
            :m_ptr(std::move(v.m_ptr)) {}

        template<typename U, typename V = std::enable_if_t<
            std::is_base_of_v<T, U> &&
            !IsDynValue<U>::value>,
            typename... Args>
        explicit DynValue(std::in_place_type_t<U>, Args&&... args)
            :m_ptr{ std::make_unique<U>(std::forward<Args>(args)...) } {};

        DynValue(const DynValue& other) {
            if (!other) return;
            m_ptr = detail::clone(*other.m_ptr.get());
        }

        DynValue(DynValue&& other) noexcept = default;

        DynValue& operator=(const DynValue& other) {
            if (std::addressof(other) == this) {
                return *this;
            }
            if (!other) {
                m_ptr.reset();
                return *this;
            }
            m_ptr = detail::clone<T>(other.m_ptr.get());
            return *this;
        }

        DynValue& operator=(DynValue&& other) noexcept = default;

        void swap(DynValue& p) noexcept
        {
            using std::swap;
            swap(m_ptr,other.m_ptr);
        }

        operator std::unique_ptr<T>() && noexcept {
            return std::move(m_ptr);
        }

        explicit operator bool() const noexcept { return m_ptr.operator bool(); }

        const T* get() const noexcept
        {
            return m_ptr.get();
        }

        T* get() noexcept
        {
            return m_ptr.get();
        }

        const T* operator->() const
        {
            assert(m_ptr);
            return m_ptr.get();
        }

        const T& operator*() const
        {
            assert(*this);
            return *m_ptr;
        }

        T* operator->()
        {
            assert(*this);
            return m_ptr.get();
        }

        T& operator*()
        {
            assert(*this);
            return *m_ptr;
        }

        bool operator==(std::nullptr_t other) const noexcept {
            return m_ptr != nullptr;
        }
        bool operator!=(std::nullptr_t other) const noexcept {
            return m_ptr == nullptr;
        }
    };

    template<typename T, typename U>
    bool operator==(const DynValue<T>& lhs, const U* rhs) {
        static_assert(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>, "std::is_base_of_v<T,U> || std::is_base_of_v<U,T>");
        if (lhs && rhs) {
            return lhs->equal(rhs);
        }
        return (lhs.get() == rhs);
    }

    template<typename T, typename U>
    bool operator!=(const DynValue<T>& lhs, const U* rhs) {
        return !(lsh == rhs);
    }

    template<typename T, typename U>
    bool operator==(const T* lhs, const DynValue<U>& rhs) {
        return rhs == lhs;
    }

    template<typename T, typename U>
    bool operator!=(const T* lhs, const DynValue<U>& rhs) {
        return rhs != lhs;
    }

    template<typename T>
    bool operator==(const T* lhs, const DynValue<T>& rhs) {
        if (lhs && rhs) {
            return lhs->equal(rhs.get());
        }
        return lhs == rhs.get();
    }

    template<typename T>
    bool operator==(const DynValue<T>& lhs, const DynValue<T>& rhs) {
        return lhs == rhs.get();
    }

    template<typename T>
    bool operator!=(const DynValue<T>& lhs, const DynValue<T>& rhs) {
        return !(lhs == rhs);
    }

    template<typename T>
    bool operator<(const DynValue<T>& lhs, const DynValue<T>& rhs) {
        if (lhs && rhs) {
            return lhs->less(rhs.get());
        }
        return false;
    }

    template <class T, class U = T, class... Args>
    DynValue<T> MakeDynValue(Args &&...args)
    {
        return DynValue<T>{std::in_place_type_t<U>{}, std::forward<Args>(args)...};
    }

    template <class T>
    constexpr void swap(DynValue<T>& lhs, DynValue<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}
