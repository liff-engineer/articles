#pragma once

#include <memory>
#include <vector>

namespace abc
{
    template<typename... Ts>
    class RegistryTemplate;

    template<typename T>
    class RegistryTemplate<std::unique_ptr<T>> {
    public:
        std::size_t add(std::unique_ptr<T> e) {
            m_elements.emplace_back(std::move(e));
            return m_elements.size();
        }

        void        remove(const T* vp) {
            for (auto& e : m_elements) {
                if (e.get() == vp) {
                    e = nullptr;
                }
            }
        }

        void        remove(std::size_t k) {
            if (k == 0 || k > m_elements.size()) return;
            m_elements[k - 1] = nullptr;
        }

        auto begin() const { return m_elements.begin(); }
        auto end() const { return m_elements.end(); }
    private:
        std::vector<std::unique_ptr<T>> m_elements;
    };

    template<typename K, typename T>
    class RegistryTemplate<std::pair<K, T>> {
    public:
        template<typename U>
        std::size_t add(U&& k, T&& v) {
            m_elements.emplace_back(std::make_pair(std::forward<U>(k), std::move(v)));
            return m_elements.size();
        }

        template<typename U>
        void remove(U&& k)
        {
            for (auto& e : m_elements) {
                if (e.first == k) {
                    e.first = K{};
                    e.second = T{};
                }
            }
        }

        void remove(std::size_t k) {
            if (k == 0 || k > m_elements.size()) return;
            m_elements[k - 1] = std::make_pair(K{}, T{});
        }


        auto begin() const { return m_elements.begin(); }
        auto end() const { return m_elements.end(); }
    private:
        std::vector<std::pair<K, T>> m_elements;
    };
}
