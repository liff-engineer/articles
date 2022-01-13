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
        virtual std::unique_ptr<value_array_base> clone() const = 0;
    };

    /// @brief 内存连续且一直有效的值数组(遍历性能略差)
    /// @tparam T 
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

        std::size_t size() const noexcept { return m_pointers.size(); }

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
            if (id < 0 || id >= m_pointers.size()) return nullptr;
            return m_pointers[id];
        }

        void remove(int id) noexcept override {
            assert(id >= 0 && id < m_pointers.size());
            auto&& pointer = m_pointers[id];
            if (pointer != nullptr) {
                std::exchange(*pointer, T{});
                pointer = nullptr;
            }
        }

        std::unique_ptr<value_array_base> clone() const override {
            //clone时计算出合适的大小,避免重复内存申请
            std::size_t n = 0;
            for (std::size_t i = 0; i < m_pointers.size(); i++) {
                if (m_pointers[i] != nullptr) {
                    n++;
                }
            }
            auto result = std::make_unique<value_array<T>>(n);

            //指针直接设置为相同大小
            result->m_pointers.resize(m_pointers.size(), nullptr);
            //填充数据及指针
            auto dst = result->m_blocks.back().get();
            for (std::size_t i = 0; i < m_pointers.size(); i++) {
                if (m_pointers[i] != nullptr) {
                    dst->emplace_back(T{ *m_pointers[i] });
                    result->m_pointers[i] = std::addressof(dst->back());
                }
            }
            return result;
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

        std::add_pointer_t<T> at(std::size_t i) { return m_pointers.at(i); }
        std::add_pointer_t<T> operator[](std::size_t i) noexcept { return m_pointers[i]; }
    private:
        std::vector<std::unique_ptr<std::vector<T>>> m_blocks;
        std::vector<std::add_pointer_t<T>> m_pointers;
    };

    class repository;

    /// @brief 包含相应值数组地址的原型,用来访问特定类型的值数组
    /// @tparam ...Ts 
    template<typename... Ts>
    class archetype {
        repository* m_repo{};
        std::tuple<std::add_pointer_t<value_array<Ts>>...> m_stores{};
    public:
        archetype() = default;
        explicit archetype(repository& repo);

        inline repository* repo() const noexcept { return m_repo; }

        explicit operator bool() const noexcept { return (m_repo != nullptr); }

        template<typename T>
        std::add_pointer_t<value_array<T>> store() const noexcept;

        template<typename T>
        std::add_pointer_t<value_array<T>> store() noexcept;
    };

    template<typename T>
    struct logic_type_traits : std::false_type {};

    template<typename T>
    using   is_logic_type = logic_type_traits<T>;

    /// @brief 逻辑类型值绑定
    template<typename T, typename U>
    struct  logic_value_tie {
        U& v;
    };

    /// @brief 逻辑类型指针绑定
    template<typename T, typename U>
    struct logic_pointer_tie {
        U*& vp;
    };

    template<typename T,typename U>
    auto tie(U& v) {
        return logic_value_tie<T, U>{v};
    }

    template<typename T, typename U>
    auto tie(U*& vp) {
        return logic_pointer_tie<T, U>{vp};
    }

    namespace detail {
        /// @brief 实体投射器,用来投射实体信息到结构体或已有变量
        struct projector {

            template<typename E, typename U>
            static void on(E& e, U*& vp) { vp = e.view<U>(); }

            template<typename E, typename U>
            static void on(E& e, U& v) {
                if (auto vp = e.view<U>()) { v = *vp; }
            }

            template<typename E, typename T, typename U>
            static void on(E& e, logic_pointer_tie<T, U>& o) {
                o.vp = e.view<T>();
            }

            template<typename E, typename T, typename U>
            static void on(E& e, logic_value_tie<T, U>& o) {
                if (auto vp = e.view<T>()) {
                    o.v = *vp;
                }
            }
        };
    }

    /// @brief 示例用逻辑类型泛型组件
    template<typename T, typename Tag>
    struct logic_type {
        T v{};
    };

    template<typename T, typename U>
    struct logic_type_traits<logic_type<T, U>> : std::true_type {
        using underlying_type = T;

        T& get(logic_type<T, U>& o) const noexcept { return o.v; }
        const T& get(const logic_type<T, U>& o) const noexcept { return o.v; }
    };

    /// @brief 实体视图
    /// @tparam ...Ts 
    template<typename... Ts>
    class entity_view {
        archetype<Ts...> m_op;
        int         m_id{ -1 };
    public:
        entity_view() = default;
        entity_view(archetype<Ts...> op, int id) :m_op(std::move(op)), m_id(id) {};
        entity_view(repository& repo, int id) :m_op(repo), m_id(id) {};

        inline int  id() const noexcept { return m_id; };
        inline void bind(int id) noexcept { m_id = id; };
        inline repository* repo() const noexcept { return m_op.repo(); }

        explicit operator bool() const noexcept;

        template<typename T, typename... Us>
        bool exist() const noexcept {
            auto h = m_op.store<T>();
            if (h == nullptr || !h->exist(id())) return false;
            if constexpr (sizeof...(Us) > 0) {
                return exist<Us...>();
            }
            return true;
        }

        template<typename T, typename... Args>
        void assign(T&& v, Args&&... args) {
            auto h = m_op.store<std::remove_cv_t<T>>();
            h->assign(id(), std::forward<T>(v));

            if constexpr (sizeof...(Args) > 0) {
                assign(std::forward<Args>(args)...);
            }
        }

        template<typename T, typename... Us, typename E>
        void merge(const E& e) {
            if (auto v = e.view<T>()) {
                m_op.store<T>()->assign(id(), *v);
            }
            if constexpr (sizeof...(Us) > 0) {
                merge<Us...>(e);
            }
        }

        template<typename T>
        T* view_impl() const noexcept {
            if (auto h = m_op.store<T>()) { return h->view(id()); }
            return nullptr;
        }

        template<typename T>
        auto view() noexcept {
            if constexpr (!is_logic_type<T>::value) {
                return view_impl<T>();
            }
            else
            {
                using result_type = typename logic_type_traits<T>::underlying_type;
                result_type* result = nullptr;
                if (auto vp = view_impl<T>()) {
                    result = std::addressof(logic_type_traits<T>{}.get(*vp));
                }
                return result;
            }
        }

        template<typename T>
        auto view() const noexcept {
            if constexpr (!is_logic_type<T>::value) {
                return view_impl<T>();
            }
            else
            {
                using result_type = typename logic_type_traits<T>::underlying_type;
                result_type* result = nullptr;
                if (auto vp = view_impl<T>()) {
                    result = std::addressof(logic_type_traits<T>{}.get(*vp));
                }
                return result;
            }
        }

        template<typename T>
        void remove() noexcept {
            if (auto h = m_op.store<T>()) { h->remove(id()); }
        }

        template<typename Fn, typename... Us>
        auto apply(Fn&& fn, Us&&... args) noexcept {
            return fn(std::forward<Us>(args)..., view<Ts>()...);
        }

        template<typename Fn, typename... Us>
        auto apply(Fn&& fn, Us&&... args) const noexcept {
            return fn(std::forward<Us>(args)..., view<Ts>()...);
        }


        /// @brief 将包含的值投射到参数(引用、指针引用形式)上
        template<typename... Args>
        void project(Args&... args) noexcept {
            (detail::projector::on(*this, args), ...);
        }

        friend bool operator==(entity_view const& lhs, entity_view const& rhs) noexcept {
            return (lhs.id() == rhs.id()) && (lhs.repo() == rhs.repo());
        }

        friend bool operator!=(entity_view const& lhs, entity_view const& rhs) noexcept {
            return (lhs.id() != rhs.id()) || (lhs.repo() != rhs.repo());
        }

        template<typename U, typename E = std::enable_if_t<std::is_constructible_v<U, repository&, int>>>
        operator U() const {
            return U{ *m_op.repo(),m_id };
        }
    };

    /// @brief 实体迭代器
    /// @tparam ...Ts 
    template<typename... Ts>
    struct entity_iterator
    {
        using difference_type = std::ptrdiff_t;
        using value_type = entity_view<Ts...>;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;

        explicit entity_iterator(repository& repo, int id) : m_obj(repo, id) {};

        bool operator==(const entity_iterator& rhs) const noexcept { return (m_obj == rhs.m_obj); }
        bool operator!=(const entity_iterator& rhs) const noexcept { return !(*this == rhs); }

        reference operator*() noexcept { return m_obj; }
        pointer  operator->() noexcept { return std::addressof(m_obj); }

        entity_iterator& operator++() noexcept;

        entity_iterator operator++(int) noexcept {
            entity_iterator ret = *this;
            ++* this;
            return ret;
        }

        entity_iterator& operator--() noexcept;
        entity_iterator operator--(int) noexcept {
            entity_iterator ret = *this;
            --* this;
            return ret;
        }
    private:
        entity_view<Ts...> m_obj;
    };

    /// @brief 存储库
    class repository
    {
        struct typed_value_store {
            std::string_view code;
            std::unique_ptr<value_array_base> values;
        };
        std::vector<typed_value_store> m_stores;
        std::vector<int>  m_entitys;
    private:
        template<typename... Ts>
        friend  class archetype;

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
            m_stores.emplace_back(typed_value_store{ type_code_of<T>(),
                    std::make_unique<value_array<T>>(initialize_capacity)
                });
            return static_cast<value_array<T>*>(m_stores.back().values.get());
        }
    public:
        static constexpr int invalid_id = -1;

        repository() = default;
        repository(const repository& other)
            :m_entitys(other.m_entitys)
        {
            m_stores.reserve(other.m_stores.size());
            for (auto& o : other.m_stores) {
                m_stores.emplace_back(typed_value_store{ o.code,o.values->clone() });
            }
        }

        repository& operator=(const repository& other) {
            if (std::addressof(other) != this) {
                m_entitys = other.m_entitys;
                m_stores.reserve(other.m_stores.size());
                for (auto& o : other.m_stores) {
                    m_stores.emplace_back(typed_value_store{ o.code,o.values->clone() });
                }
            }
            return *this;
        }

        repository(repository&& other) noexcept
            :m_stores(std::move(other.m_stores)), m_entitys(std::move(other.m_entitys))
        {};
        repository& operator=(repository&& other) noexcept {
            if (std::addressof(other) != this) {
                m_stores = std::move(other.m_stores);
                m_entitys = std::move(other.m_entitys);
            }
            return *this;
        }

        inline bool valid(int id) const noexcept {
            if (id < 0 || id >= m_entitys.size()) return false;
            return m_entitys[id] == id;
        }

        inline std::size_t  size() const noexcept { return m_entitys.size(); }

        bool   empty() const noexcept {
            for (std::size_t i = 0; i < m_entitys.size(); i++) {
                if (m_entitys[i] == i) return false;
            }
            return true;
        }

        void   resize(std::size_t n) {
            auto number = m_entitys.size();
            if (n > number) {
                m_entitys.resize(n);
                for (std::size_t i = number; i < n; i++) {
                    m_entitys[i] = static_cast<int>(i);
                }
            }
            else {
                for (std::size_t i = n; i < number; i++) {
                    destory(static_cast<int>(i));
                }
                m_entitys.resize(n);
            }
        }

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

        entity_view<> at(int id) { return entity_view<>{*this, id}; }

        /// @brief 根据提供的数据组件值查找对应实体(只负责找到第一个)
        template<typename T>
        entity_view<> find(const T& v) {
            if (auto store = value_array_of<std::remove_cv_t<T>>()) {
                for (std::size_t i = 0; i < store->size(); i++) {
                    if (auto vp = (*store)[i]) {
                        if (*vp == v) {
                            return entity_view<>{*this, static_cast<int>(i)};
                        }
                    }
                }
            }
            return entity_view<>{*this, invalid_id};
        }

        /// @brief 根据指定的数据组件类型按条件查找对应实体(只负责找到第一个)
        template<typename T, typename Fn>
        entity_view<> find_if(Fn&& fn) {
            if (auto store = value_array_of<T>()) {
                for (std::size_t i = 0; i < store->size(); i++) {
                    if (auto vp = (*store)[i]) {
                        if (fn(*vp)) {
                            return entity_view<>{*this, static_cast<int>(i)};
                        }
                    }
                }
            }
            return entity_view<>{*this, invalid_id};
        }

        template<typename... Ts>
        struct iterators
        {
            using iterator = entity_iterator<Ts...>;
            using revertse_iterator = std::reverse_iterator<iterator>;

            explicit iterators(repository& repo) :m_repo(std::addressof(repo)) {};

            iterator begin() noexcept {
                auto n = m_repo->size();
                for (std::size_t i = 0; i < n; i++) {
                    if (m_repo->valid(i)) {
                        return iterator{ *m_repo, static_cast<int>(i) };
                    }
                }
                return  iterator{ *m_repo, static_cast<int>(m_repo->size()) };
            }

            iterator end() noexcept { return iterator{ *m_repo, static_cast<int>(m_repo->size()) }; }

            revertse_iterator rbegin() noexcept { return revertse_iterator{ end() }; }
            revertse_iterator rend() noexcept { return revertse_iterator{ begin() }; }
        private:
            repository* m_repo{};
        };

        template<typename... Ts>
        auto iterator() noexcept { return iterators<Ts...>{*this}; }

        auto begin() noexcept { return iterators<>{*this}.begin(); }
        auto end() noexcept { return iterators<>{*this}.end(); }
        auto rbegin() noexcept { return iterators<>{*this}.rbegin(); }
        auto rend() noexcept { return iterators<>{*this}.rend(); }
    };

    template<typename ...Ts>
    inline archetype<Ts...>::archetype(repository& repo)
        :m_repo(std::addressof(repo)), m_stores([&] { return std::make_tuple(repo.value_array_of<Ts>()...); }()) {};

    template<typename ...Ts>
    template<typename T>
    inline std::add_pointer_t<value_array<T>> archetype<Ts...>::store() const noexcept
    {
        if constexpr (std::disjunction_v<std::is_same<T, Ts>...>) {
            return std::get<std::add_pointer_t<value_array<T>>>(m_stores);
        }
        else {
            return m_repo->value_array_of<T>();
        }
    }

    template<typename ...Ts>
    template<typename T>
    inline std::add_pointer_t<value_array<T>> archetype<Ts...>::store() noexcept
    {
        if constexpr (std::disjunction_v<std::is_same<T, Ts>...>) {
            if (auto result = std::get<std::add_pointer_t<value_array<T>>>(m_stores)) {
                return result;
            }
            auto result = m_repo->alloc_value_array_of<T>();
            std::get<std::add_pointer_t<value_array<T>>>(m_stores) = result;
            return result;
        }
        else {
            if (auto result = m_repo->value_array_of<T>()) {
                return result;
            }
            return m_repo->alloc_value_array_of<T>();
        }
    }

    template<typename ...Ts>
    inline entity_view<Ts...>::operator bool() const noexcept
    {
        return (m_op.repo() != nullptr) && (m_op.repo()->valid(m_id));
    }

    template<typename ...Ts>
    inline entity_iterator<Ts...>& entity_iterator<Ts...>::operator++() noexcept
    {
        auto repo = m_obj.repo();
        auto id = m_obj.id();
        auto n = static_cast<int>(repo->size());
        while (id < n) {
            id++;
            if (repo->valid(id)) {
                break;
            }
        }
        m_obj.bind(id);
        return *this;
    }

    template<typename ...Ts>
    inline entity_iterator<Ts...>& entity_iterator<Ts...>::operator--() noexcept
    {
        auto repo = m_obj.repo();
        auto id = m_obj.id();
        while (id > 0) {
            id--;
            if (repo->valid(id)) {
                break;
            }
        }
        m_obj.bind(id);
        return *this;
    }
}
