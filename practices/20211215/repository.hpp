#pragma once
#include <vector>
#include <memory>
#include <string_view>
#include <tuple>
#include <cassert>

namespace abc {

    template<typename... Ts>
    constexpr std::string_view type_code_of() noexcept {
#ifdef __clang__
        std::string_view result{ __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
        std::string_view result{ __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
        std::string_view result{ __FUNCSIG__ };
        auto l = result.find("type_code_of") + 13;//跳出自身长度
        auto r = result.find_last_of('>');
        result = result.substr(l, r - l);
#else
#error "Unsupported compiler"
#endif
        return result;
    }

    struct value_array_base {
        virtual ~value_array_base() = default;
        virtual void remove(int id) noexcept = 0;
    };

    template<typename T>
    struct value_array final :public value_array_base
    {
        explicit value_array(std::size_t capacity)
        {
            auto block = std::make_unique<std::vector<T>>();
            block->reserve(capacity);
            m_blocks.emplace_back(std::move(block));
            m_pointers.reserve(capacity);
        }

        std::size_t size() const noexcept {
            return m_pointers.size();
        }

        bool exist(int id) const noexcept {
            if (id < 0 || id >= m_pointers.size()) return false;
            return m_pointers[id] != nullptr;
        }

        void assign(int id, T&& v) {
            assert(id >= 0);
            if (id + 1 >= m_pointers.size()) {
                m_pointers.resize(id + 1, nullptr);
            }

            auto&& pointer = m_pointers[id];
            if (pointer == nullptr) {
                pointer = alloc(std::forward<T>(v));
            }
            else {
                *pointer = T{ std::forward<T>(v) };
            }
        }

        std::add_pointer_t<T> view(int id) noexcept {
            assert(id >= 0 && id < m_pointers.size());
            return m_pointers[id];
        }

        void remove(int id) noexcept override {
            assert(id >= 0 && id < m_pointers.size());
            m_pointers[id] = nullptr;
        }

        std::add_pointer_t<T> alloc(T&& v = {}) {
            auto  current = m_blocks.back().get();
            const auto capacity = current->capacity();
            const auto size = current->size();
            if (size + 1 > capacity) {
                auto block = std::make_unique<std::vector<T>>();
                block->reserve(capacity * 2);
                m_blocks.emplace_back(std::move(block));
                current = m_blocks.back().get();
            }
            current->emplace_back(std::forward<T>(v));
            return std::addressof(current->back());
        }

        template<typename Fn>
        void foreach(Fn&& fn) {
            for (std::size_t i = 0; i < m_pointers.size(); i++) {
                if (m_pointers[i] == nullptr) continue;
                fn(i, *m_pointers[i]);
            }
        }
    private:
        std::vector<std::unique_ptr<std::vector<T>>> m_blocks;
        std::vector<std::add_pointer_t<T>> m_pointers;
    };

    class repository
    {
        struct typed_value_store {
            std::string_view code;
            std::unique_ptr<value_array_base> values;
        };
        std::vector<typed_value_store> m_stores;
        std::vector<int>  m_entitys;
    private:
        template<typename T>
        value_array<T>* value_array_of() const noexcept {
            for (auto&& o : m_stores) {
                if (o.code == type_code_of<T>()) {
                    return static_cast<value_array<T>*>(o.values.get());
                }
            }
            return nullptr;
        }

        template<typename T>
        value_array<T>* alloc_value_array_of() {
            constexpr auto initialize_capacity = 8;
            m_stores.emplace_back(typed_value_store{
                    type_code_of<T>(),
                    std::make_unique<value_array<T>>(initialize_capacity)
                });
            return static_cast<value_array<T>*>(m_stores.back().values.get());
        }

        inline bool valid(int id) const noexcept {
            if (id < 0 || id >= m_entitys.size()) return false;
            return m_entitys[id] != invalid_id;
        }

        template<typename... Ts>
        class archetype {
            repository* m_repo{};
            std::tuple<std::add_pointer_t<value_array<Ts>>...> m_stores;

            template<typename T>
            static void fill(value_array<T>*& h, repository& repo) {
                if (!(h = repo.value_array_of<T>())) {
                    h = repo.alloc_value_array_of<T>();
                }
            }

            static std::tuple<std::add_pointer_t<value_array<Ts>>...> build_stores(repository& repo) {
                std::tuple<std::add_pointer_t<value_array<Ts>>...> result{};
                std::apply([&](auto&&... args) { (fill(args, repo), ...); }, result);
                return result;
            }

            template<typename T>
            static constexpr bool contains() {
                return std::disjunction_v<std::is_same<T, Ts>...>;
            }
        public:
            explicit archetype(repository& repo)
                :m_repo(std::addressof(repo)), m_stores(build_stores(repo)) {};

            template<typename T>
            constexpr auto store() const noexcept {
                static_assert(contains<T>(), "archetype don't contain your type!");
                return std::get<std::add_pointer_t<value_array<T>>>(m_stores);
            }

            template<typename T>
            constexpr auto store() noexcept {
                static_assert(contains<T>(), "archetype don't contain your type!");
                return std::get<std::add_pointer_t<value_array<T>>>(m_stores);
            }
        };

        template<>
        class archetype<> {
            repository* m_repo{};
        public:
            explicit archetype(repository& repo)
                :m_repo(std::addressof(repo)) {};

            template<typename T>
            constexpr auto store() const noexcept {
                return m_repo->value_array_of<T>();
            }

            template<typename T>
            constexpr auto store() noexcept {
                if (auto h = m_repo->value_array_of<T>()) {
                    return h;
                }
                return m_repo->alloc_value_array_of<T>();
            }
        };

        template<typename... Ts>
        struct foreach_helper {
            int id;
            std::tuple<std::add_pointer_t<Ts>...> members;

            explicit operator bool() const noexcept {
                return std::apply([](auto&&... args)->bool { return ((args != nullptr) &&...); }, members);
            }

            template<typename Fn>
            auto run(Fn&& fn) {
                return  std::apply([&](auto&&... args) { return fn(id, (*args)...); }, members);
            }

            template<typename T>
            static T* view(repository const& repo, int id) {
                if (auto h = repo.value_array_of<T>()) {
                    return h->view(id);
                }
                return nullptr;
            }

            static foreach_helper build(repository const& repo, int id) {
                return { id, std::make_tuple(view<Ts>(repo,id)...) };
            }
        };
    public:
        static constexpr int invalid_id = -1;

        template<typename... Ts>
        class entity_view {
            repository* m_repo{};
            int         m_id{ invalid_id };
            archetype<Ts...> m_op;
        public:
            entity_view() = default;
            entity_view(repository& repo, int id, archetype<Ts...> op)
                :m_repo(std::addressof(repo)), m_id(id), m_op(std::move(op)) {};

            entity_view(repository& repo, int id)
                :m_repo(std::addressof(repo)), m_id(id), m_op(repo) {};

            int id() const noexcept { return m_id; };

            explicit operator bool() const noexcept {
                return (m_repo != nullptr) && (m_repo->valid(m_id));
            }

            template<typename T>
            bool exist() const noexcept {
                auto h = m_op.store<T>();
                return (h != nullptr) && (h->exist(id()));
            }

            template<typename T, typename... Args>
            void assign(T&& v, Args&&... args) {
                auto h = m_op.store<std::remove_cv_t<T>>();
                assert(h != nullptr);
                h->assign(id(), std::forward<T>(v));

                if constexpr (sizeof...(Args) > 0) {
                    assign(std::forward<Args>(args)...);
                }
            }

            template<typename T>
            T& view() noexcept {
                assert(this->operator bool());
                auto h = m_op.store<std::remove_cv_t<T>>();
                assert(h != nullptr);
                auto pointer = h->view(id());
                assert(pointer != nullptr);
                return *pointer;
            }

            template<typename T>
            const T& view() const noexcept {
                assert(this->operator bool());
                auto h = m_op.store<std::remove_cv_t<T>>();
                assert(h != nullptr);
                auto pointer = h->view(id());
                assert(pointer != nullptr);
                return *pointer;
            }

            template<typename T>
            void remove() noexcept {
                auto h = m_op.store<T>();
                assert(h != nullptr);
                h->remove(id());
            }

            template<typename U, typename E = std::enable_if_t<std::is_constructible_v<U, repository&, int>>>
            operator U() const {
                return U{ *m_repo,m_id };
            }
        };
    public:
        entity_view<> create() {
            m_entitys.emplace_back(static_cast<int>(m_entitys.size()));
            return entity_view<>{*this, static_cast<int>(m_entitys.size() - 1)};
        }

        void destory(int id) noexcept {
            if (!valid(id)) return;
            m_entitys[id] = invalid_id;
            for (auto&& o : m_stores) {
                o.values->remove(id);
            }
        }

        template<typename... Ts>
        entity_view<Ts...> find(int id) {
            return entity_view<Ts...>{*this, id};
        }

        template<typename... Ts, typename Fn>
        void each(Fn&& fn) {
            archetype<Ts...> op{ *this };
            for (auto& v : m_entitys) {
                if (!valid(v)) continue;
                entity_view<Ts...> e{ *this, v, op };
                fn(e);
            }
        }

        template<typename... Ts, typename Fn>
        void foreach(Fn&& fn) {
            static_assert(sizeof...(Ts) > 0, "provide require type list");
            if constexpr (sizeof...(Ts) == 1) {
                if (auto h = value_array_of<T>()) {
                    h->foreach(std::forward<Fn>(fn));
                }
            }
            else
            {
                for (auto& v : m_entitys) {
                    if (!valid(v)) continue;
                    if (auto obj = foreach_helper<Ts...>::build(*this, v)) {
                        obj.run(std::forward<Fn>(fn));
                    }
                }
            }
        }
    };
}
