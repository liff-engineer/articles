#pragma once
#include <memory>
#include <vector>
#include <iostream>

class dynamic_tuple
{
public:
    class value
    {
        struct value_base {
            virtual ~value_base() = default;
            virtual std::size_t type_code() const noexcept = 0;
            virtual std::unique_ptr<value_base> clone() const noexcept = 0;
        };

        template<typename T>
        struct value_impl final :public value_base
        {
            T v;

            template<typename U>
            explicit value_impl(U&& u)
                :v(std::forward<U>(u)) {};

            std::size_t type_code() const noexcept { return typeid(T).hash_code(); }
            std::unique_ptr<value_base> clone() const noexcept { return std::make_unique<value_impl<T>>(v); };
        };

        std::unique_ptr<value_base> m_impl;

        template<typename T>
        static std::unique_ptr<value_base> create(T&& v) noexcept {
            return std::make_unique<value_impl<std::remove_cv_t<T>>>(std::forward<T>(v));
        }
    public:
        template<typename T>
        value(T&& v)
            :m_impl(create(std::forward<T>(v))) {
            std::cout << "value(T&&)\n";
        };

        value(const value& other)
            :m_impl(other.m_impl->clone()) {
            std::cout << "value(const value&)\n";
        };

        value(value&& other)
            :m_impl(std::move(other.m_impl)) {
            std::cout << "value(value&&)\n";
        };

        value& operator=(const value& other) {
            std::cout << "value& operator=(const value &)\n";
            if (std::addressof(other) != this) {
                m_impl = other.m_impl->clone();
            }
            return *this;
        }

        value& operator=(value&& other) noexcept {
            std::cout << "value& operator=(value &&)\n";
            if (std::addressof(other) != this) {
                m_impl = std::move(other.m_impl);
            }
            return *this;
        }

        template<typename T>
        T* as() noexcept {
            if (m_impl->type_code() == typeid(T).hash_code()) {
                return std::addressof(static_cast<value_impl<T>*>(m_impl.get())->v);
            }
            return nullptr;
        }

        template<typename T>
        const T* as() const noexcept {
            if (m_impl->type_code() == typeid(T).hash_code()) {
                return std::addressof(static_cast<const value_impl<T>*>(m_impl.get())->v);
            }
            return nullptr;
        }

        void swap(value& other) noexcept {
            using std::swap;
            swap(m_impl, other.m_impl);
        }

        explicit operator bool() const noexcept { return m_impl.operator bool(); }
    };

    template<typename T>
    void push_back(T&& v) {
        m_values.emplace_back(std::forward<T>(v));
    }

    template<typename T>
    T* at() noexcept {
        for (auto&& v : m_values) {
            if (auto vp = v.as<T>()) {
                return vp;
            }
        }
        return vp;
    }

    void reserve(std::size_t n) {
        m_values.reserve(n);
    }
private:
    std::vector<value> m_values;
};
