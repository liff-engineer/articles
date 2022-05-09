#pragma once

#include "RepoImpl.hpp"
#include <string>

namespace abc
{
    inline namespace v1
    {
        class Repository;
        class EntityView final {
        public:
            using difference_type = std::ptrdiff_t;
            using value_type = EntityView;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::forward_iterator_tag;
        public:
            EntityView() = default;
            EntityView(Repository& repo, int key)
                :m_repo(std::addressof(repo)), m_key(key) {};

            inline int  key() const noexcept { return m_key; }
            inline void bind(int key) { m_key = key; }
            inline Repository* container() const noexcept { return m_repo; }

            explicit operator bool() const noexcept;

            template<typename T, typename... Us>
            bool contains() const noexcept {
                auto h = store<T>();
                if (h == nullptr || !h->contains(key())) return false;
                if constexpr (sizeof...(Us) > 0) {
                    return contains<Us...>();
                }
                return true;
            }

            template<typename T, typename... Args>
            void emplace(T&& v, Args&&... args) {
                auto h = store<std::remove_cv_t<T>>();
                h->emplace(key(), std::forward<T>(v));

                if constexpr (sizeof...(Args) > 0) {
                    emplace(std::forward<Args>(args)...);
                }
            }

            template<typename T>
            const T* view() const noexcept {
                if (auto h = store<T>()) { return h->at(key()); }
                return nullptr;
            }

            template<typename T, typename... Us>
            void erase() noexcept {
                if (auto h = store<T>()) { h->erase(key()); }

                if constexpr (sizeof...(Us) > 0) {
                    erase<Us...>();
                }
            }

            friend bool operator==(EntityView const& lhs, EntityView const& rhs) noexcept {
                return (lhs.key() == rhs.key()) && (lhs.container() == rhs.container());
            }

            friend bool operator!=(EntityView const& lhs, EntityView const& rhs) noexcept {
                return !(lhs == rhs);
            }
        public:
            void swap(EntityView& other) {
                using std::swap;
                swap(m_repo, other.m_repo);
                swap(m_key, other.m_key);
            }

            reference operator*() noexcept {
                return *this;
            }
            pointer  operator->() noexcept {
                return this;
            }

            EntityView& operator++() noexcept {
                forward();
                return *this;
            };

            EntityView operator++(int) noexcept {
                EntityView ret = *this;
                ++* this;
                return ret;
            }
        private:
            template<typename T>
            std::add_pointer_t<im::ValueArray<T>> store() const noexcept;

            template<typename T>
            std::add_pointer_t<im::ValueArray<T>> store();

            void forward() noexcept;
        private:
            Repository* m_repo{ nullptr };
            int         m_key{ -1 };
        };

        class Repository final {
            static constexpr char  Empty = '0';
            static constexpr char  Valid = '1';
        public:
            Repository() = default;
            Repository(const Repository & other);
            Repository& operator=(const Repository & other);
            Repository(Repository && other) noexcept;
            Repository& operator=(Repository && other) noexcept;

            bool contains(std::size_t key) const noexcept {
                if (key >= m_entitys.size()) return false;
                return m_entitys[key] != Empty;
            }

            std::size_t size() const noexcept {
                return m_entitys.size();
            }

            bool empty() const noexcept {
                return (m_entitys.find_first_not_of(Empty) != m_entitys.npos);
            }

            void resize(std::size_t n) {
                auto number = m_entitys.size();
                if (n > number) {
                    m_entitys.resize(n, Valid);
                }
                else {
                    for (std::size_t i = n; i < number; i++) {
                        erase(i);
                    }
                    m_entitys.resize(n);
                }
            }

            void erase(std::size_t key) noexcept {
                if (!contains(key)) return;
                m_entitys[key] = Empty;
                for (auto& o : m_components) {
                    if (o) {
                        o->erase(key);
                    }
                }
            }

            template<typename... Args>
            EntityView emplace_back(Args&&... args) {
                m_entitys.push_back(Valid);
                auto result = EntityView{ *this,int(m_entitys.size() - 1) };
                if constexpr (sizeof...(Args) > 0) {
                    result.emplace(std::forward<Args>(args)...);
                }
                return result;
            }

            EntityView at(std::size_t key) noexcept {
                return EntityView{ *this,(int)key };
            }

            EntityView operator[](std::size_t key) noexcept {
                return EntityView{ *this,(int)key };
            }

            auto begin() noexcept {
                EntityView first{ *this,-1 };
                first++;
                return first;
            }

            auto end() noexcept {
                return EntityView{ *this,int(m_entitys.size()) };
            }

            template<typename T>
            auto values() noexcept {
                return im::ValueArrayProxy<T>{ComponentOf<T>()};
            }

            template<typename T>
            EntityView find(const T& v) {
                for (auto&& o : values<std::remove_cv_t<T>>()) {
                    if (o.value() == v) {
                        return EntityView{ *this,o.key() };
                    }
                }
                return EntityView{ *this,-1 };
            }
        private:
            template<typename T>
            std::add_pointer_t<im::ValueArray<T>> ComponentOf() const noexcept {
                auto idx = IndexOf<T>();
                if (idx >= m_components.size()) return nullptr;
                return static_cast<im::ValueArray<T>*>(m_components[idx].get());
            }

            template<typename T>
            std::add_pointer_t<im::ValueArray<T>> ComponentOf() {
                constexpr auto initialize_capacity = 8;
                auto idx = IndexOf<T>();
                if (idx >= m_components.size() || m_components[idx] == nullptr) {
                    m_components.resize(std::max(idx + 1, m_components.size()));
                    m_components[idx] = std::make_unique<im::ValueArray<T>>(initialize_capacity);
                }
                return static_cast<im::ValueArray<T>*>(m_components[idx].get());
            }

            template<typename T>
            std::size_t IndexOf() const {
                static auto result = IndexOf(typeid(T).name());
                return result;
            }

            bool next_valid(int& index) const noexcept {
                auto n = m_entitys.size();
                if (index == -1) {
                    index = 0;
                    if (contains(index)) {
                        return true;
                    }
                }

                if (index >= n) return false;
                do {
                    index++;
                } while (index < n && !contains(index));
                return true;
            }
        private:
            std::size_t  IndexOf(const char* code) const;
        private:
            friend class EntityView;
            std::vector<std::unique_ptr<im::IValueArray>> m_components;
            std::string      m_entitys;
        };

        template<typename T>
        std::add_pointer_t<im::ValueArray<T>> EntityView::store() const noexcept
        {
            return m_repo->ComponentOf<T>();
        }

        template<typename T>
        std::add_pointer_t<im::ValueArray<T>> EntityView::store()
        {
            return m_repo->ComponentOf<T>();
        }

        inline EntityView::operator bool() const noexcept
        {
            if (m_repo && m_key != -1) {
                return m_repo->contains(m_key);
            }
            return false;
        }

        inline void EntityView::forward() noexcept
        {
            m_repo->next_valid(m_key);
        }
    }
}
