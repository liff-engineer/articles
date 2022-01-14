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

    /// @brief 投射实现:实体和实体的值类型可以被投射为别的类型,来完成以下动作
    /// 实体->业务类型: 实体是通用性设计,代码相对难写+难读,可以定义业务类型,表达从实体读取的信息
    /// 值类型->常规类型: 有一些值类型仅仅是常规类型添加了特别的类型标识来表达业务含义,可以转换为常规类型
    template<typename T, typename E = void>
    struct project : std::false_type {};

    namespace im {
        /// @brief 值绑定
        template<typename T, typename U>
        struct  value_tie {
            U& v;
        };

        /// @brief 指针绑定
        template<typename T, typename U>
        struct pointer_tie {
            U*& vp;
        };

        template<typename E,typename T>
        void project_to(E& e, T*& vp) {
            vp = e.view<T>();
        }

        template<typename E, typename T>
        void project_to(E& e, T& v) {
            if (auto vp = e.view<T>()) {
                v = *vp;
            }
        }

        template<typename E, typename T, typename U>
        void project_to(E& e, pointer_tie<T, U>& o) {
            o.vp = e.view<T>();
        }

        template<typename E, typename T, typename U>
        void project_to(E& e, value_tie<T, U>& o) {
            if (auto vp = e.view<T>()) {
                o.v = *vp;
            }
        }
    }

    /// @brief 创建引用绑定并指定映射类型:指定读取T类型值以填充U类型的值
    template<typename T, typename U>
    auto tie(U& v) {
        return im::value_tie<T, U>{v};
    }

    /// @brief 创建指针引用绑定并指定映射类型:指定读取T类型值以填充U类型的值
    template<typename T, typename U>
    auto tie(U*& vp) {
        return im::pointer_tie<T, U>{vp};
    }

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
        std::enable_if_t<!abc::project<T>::value, T*> view() noexcept {
            if (auto h = m_op.store<T>()) { return h->view(id()); }
            return nullptr;
        }

        template<typename T>
        std::enable_if_t<!abc::project<T>::value, const T*> view() const noexcept {
            if (auto h = m_op.store<T>()) { return h->view(id()); }
            return nullptr;
        }

        template<typename T>
        std::enable_if_t<abc::project<T>::value, typename abc::project<T>::type*> view() noexcept {
            if (auto h = m_op.store<T>()) {
                if (auto vp = h->view(id())) {
                    return std::addressof(abc::project<T>::to(*vp));
                }
            }
            return nullptr;
        }

        template<typename T>
        std::enable_if_t<abc::project<T>::value, const typename abc::project<T>::type*> view() const noexcept {
            if (auto h = m_op.store<T>()) {
                if (auto vp = h->view(id())) {
                    return std::addressof(abc::project<T>::to(*vp));
                }
            }
            return nullptr;
        }

        template<typename T, typename... Us>
        void remove() noexcept {
            if (auto h = m_op.store<T>()) { h->remove(id()); }

            if constexpr (sizeof...(Us) > 0) {
                remove<Us...>();
            }
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
            (im::project_to(*this, args), ...);
        }

        template<typename T>
        std::enable_if_t<abc::project<T>::value, T> as() {
            T v{};
            typename abc::project<T>::type src{ *this };
            abc::project<T>::to(src, v);
            return v;
        }

        template<typename T>
        std::enable_if_t<abc::project<T>::value, void> project(T& v) noexcept {
            typename abc::project<T>::type src{ *this };
            abc::project<T>::to(src, v);
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


    //improve
    namespace im
    {
        /// @brief 双向迭代器
        /// @tparam T 可迭代类型,需满足一定条件(包含value_type定义,==,值与值指针,前进与后退)
        template<typename T>
        struct iterator
        {
            using difference_type = std::ptrdiff_t;
            using value_type = typename T::value_type;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = std::bidirectional_iterator_tag;

            iterator(T&& obj) :m_object(std::move(obj)) {}

            bool operator==(const iterator& rhs) const noexcept { return (m_object == rhs.m_object); }
            bool operator!=(const iterator& rhs) const noexcept { return !(*this == rhs); }

            void swap(iterator& other) {
                using std::swap;
                swap(m_object, other.m_object);
            }

            reference operator*() noexcept { return m_object.value_reference(); }
            pointer  operator->() noexcept { return m_object.value_pointer(); }

            iterator& operator++() noexcept { m_object.forward(); return *this; };

            iterator operator++(int) noexcept {
                iterator ret = *this;
                ++* this;
                return ret;
            }

            iterator& operator--() noexcept { m_object.backward(); return *this; }
            iterator operator--(int) noexcept {
                iterator ret = *this;
                --* this;
                return ret;
            }
        private:
            T   m_object;
        };

        /// @brief 虚拟容器
        /// @tparam T 可迭代类型
        template<typename T>
        struct container {
            using iterator = iterator<T>;
            using revertse_iterator = std::reverse_iterator<iterator>;

            container(T&& first, T&& last) :m_first(std::move(first)), m_last(std::move(last)) {};

            iterator begin() noexcept { return T{ m_first }; }
            iterator end() noexcept { return T{ m_last }; }

            revertse_iterator rbegin() noexcept { return revertse_iterator{ end() }; }
            revertse_iterator rend() noexcept { return revertse_iterator{ begin() }; }
        private:
            T m_first;
            T m_last;
        };
    }

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

        bool next_valid(int& id) const noexcept {
            auto n = static_cast<int>(m_entitys.size());
            if (id >= n) return false;
            while (id < n) {
                id++;
                if (valid(id)) {
                    break;
                }
            }
            return true;
        }

        bool prev_valid(int& id) const noexcept {
            if (id < 0) return false;
            while (id >= 0) {
                id--;
                if (valid(id)) {
                    break;
                }
            }
            return true;
        }
    public:
        static constexpr int invalid_id = -1;

        repository() = default;
        repository(const repository& other)
            :m_entitys(other.m_entitys)
        {
            m_stores.reserve(other.m_stores.size());
            m_stores.clear();
            for (auto& o : other.m_stores) {
                m_stores.emplace_back(typed_value_store{ o.code,o.values->clone() });
            }
        }

        repository& operator=(const repository& other) {
            if (std::addressof(other) != this) {
                m_entitys = other.m_entitys;
                m_stores.clear();
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

        entity_view<> at(int id) noexcept { return entity_view<>{*this, id}; }

        /// @brief 针对实体视图的可迭代类型
        /// @tparam ...Ts 实体视图包含的类型列表
        template<typename... Ts>
        struct iterable_view_object {
            using value_type = typename entity_view<Ts...>;
            value_type  object;

            explicit iterable_view_object(repository* repo, int id):object(*repo, id) {};

            value_type& value_reference() noexcept { return object; }
            auto value_pointer() noexcept { return std::addressof(object); }

            bool operator==(const iterable_view_object& rhs) const noexcept { return object == rhs.object; }
            bool operator!=(const iterable_view_object& rhs) const noexcept { !(*this == rhs); }

            void forward() noexcept {
                auto id = object.id();
                object.repo()->next_valid(id);
                object.bind(id);
            }

            void backward() noexcept {
                auto id = object.id();
                object.repo()->prev_valid(id);
                object.bind(id);
            }
        };

        /// @brief 遍历实体视图
        /// @tparam ...Ts 实体所包含的值类型列表
        /// @return 虚拟的实体视图容器
        template<typename... Ts>
        auto views() noexcept {
            iterable_view_object<Ts...> first{ this, invalid_id };
            first.forward();
            iterable_view_object<Ts...> last = first;
            last.object.bind(static_cast<int>(m_entitys.size()));
            return im::container<iterable_view_object<Ts...>>{std::move(first), std::move(last)};
        };

        auto begin() noexcept { return views<>().begin(); }
        auto end() noexcept { return views<>().end(); }
        auto rbegin() noexcept { return views<>().rbegin(); }
        auto rend() noexcept { return views<>().rend(); }

        /// @brief 针对实体视图可映射的类型提供的可迭代类型
        /// @tparam T 支持映射的特定类型
        template<typename T>
        struct iterable_project_view_object
        {
            using value_type = T;
            typename project<T>::type entity;
            T    logic_object{};

            explicit iterable_project_view_object(repository* repo, int id)
                :entity(*repo, id) {};

            value_type& value_reference() noexcept { return logic_object; }
            auto value_pointer() noexcept { return std::addressof(logic_object); }

            bool operator==(const iterable_project_view_object& rhs) const noexcept { return entity == rhs.entity; }
            bool operator!=(const iterable_project_view_object& rhs) const noexcept { !(*this == rhs); }

            void update() noexcept {
                logic_object = T{};
                if (entity) {
                    project<T>::to(entity, logic_object);
                }
            }

            void forward() noexcept {
                auto action = [&]()->bool {
                    auto id = entity.id();
                    auto result = entity.repo()->next_valid(id);
                    entity.bind(id);
                    if (result) {
                        update();
                    }
                    return result;
                };

                if (auto result = action()) {
                    //如果有检测是否为空的判定时,直到结束或有效
                    if constexpr (std::is_convertible_v<T, bool>) {
                        while (!logic_object) {
                            auto result = action();
                            if (!result) {
                                break;
                            }
                        }
                    }
                }
            }

            void backward() noexcept {
                auto action = [&]()->bool {
                    auto id = entity.id();
                    auto result = entity.repo()->prev_valid(id);
                    entity.bind(id);
                    if (result) {
                        update();
                    }
                    return result;
                };
                if (auto result = action()) {
                    //如果有检测是否为空的判定时,直到结束或有效
                    if constexpr (std::is_convertible_v<T, bool>) {
                        while (!logic_object) {
                            auto result = action();
                            if (!result) {
                                break;
                            }
                        }
                    }
                }
            }
        };

        /// @brief  遍历映射出来的逻辑类型
        /// @tparam T 逻辑类型
        /// @return 虚拟的逻辑类型容器
        template<typename T>
        auto project_views() noexcept {
            iterable_project_view_object<T> first{ this,invalid_id };
            first.forward();

            iterable_project_view_object<T> last = first;
            last.entity.bind(static_cast<int>(m_entitys.size()));
            return im::container<iterable_project_view_object<T>>{std::move(first), std::move(last)};
        }

        template<typename T>
        class item {
            int m_key{-1};
            T* m_value{};
        public:
            item() = default;
            explicit item(int id, T* vp)
                :m_key(id), m_value(vp) {};

            int key() const noexcept { return m_key; }
            const T& value()  const noexcept { return *m_value; };
            T& value() noexcept { return *m_value; };
        };

        template<typename T>
        struct iterable_value_object
        {
            using value_type = typename item<T>;
            value_array<T>* array;
            item<T>      value;

            explicit iterable_value_object(value_array<T>* array_arg, std::size_t i)
                :array(array_arg), value(i, nullptr) {
                if (array && value.key() < array->size()) {
                    value = item<T>{ value.key(),(*array)[value.key()] };
                }
            };

            value_type& value_reference() noexcept { return value; }
            auto value_pointer() noexcept { return std::addressof(value); }

            bool operator==(const iterable_value_object& rhs) const noexcept { return (array == rhs.array) && (value.key() == rhs.value.key()); }
            bool operator!=(const iterable_value_object& rhs) const noexcept { !(*this == rhs); }

            void forward() noexcept {
                if (!array) return;
                auto n = array->size();
                for (auto i = value.key()+1; i < n; i++) {
                    if ((*array)[i] != nullptr) {
                        value = item<T>{ static_cast<int>(i),(*array)[i] };
                        return;
                    }
                }
                value = item<T>{ static_cast<int>(n),nullptr};
            }

            void backward() noexcept {
                if (!array) return;
                auto i = value.key();
                while (i > 0) {
                    i--;
                    if ((*array)[i] != nullptr) {
                        value = item<T>{ static_cast<int>(i),(*array)[i] };
                        return;
                    }
                }
                value = item<T>{ static_cast<int>(i),(*array)[i] };
            }
        };

        /// @brief 遍历指定的值类型数组
        /// @tparam T 值类型
        /// @return 虚拟的值数组容器(value_array<T>)
        template<typename T>
        auto values() noexcept {
            auto array = this->value_array_of<T>();
            iterable_value_object<T> first{ array,0 };
            iterable_value_object<T> last{ array,array != nullptr ? array->size() : 0 };
            return im::container<iterable_value_object<T>>{std::move(first), std::move(last)};
        }

        /// @brief 根据提供的数据组件值查找对应实体(只负责找到第一个)
        template<typename T>
        entity_view<> find(const T& v) {
            for (auto&& o : values<std::remove_cv_t<T>>()) {
                if (o.value() == v) {
                    return entity_view<>{*this, o.key()};
                }
            }
            return entity_view<>{*this, invalid_id};
        }
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

    namespace examples
    {
        /// @brief 示例:带类型Tag的值类型,以保证类型安全及表意性
        template<typename T, typename Tag>
        struct logic_type {
            T v{};
        };

        /// @brief 该封装类型可以让指针按值操作,避免使用*带来困扰
        template<typename T>
        class pointer {
            T* vp{};
        public:
            pointer() = default;
            explicit pointer(T* p) :vp(p) {};

            T& operator*() noexcept { return *vp; }
            T* operator->() noexcept { return vp; }
            const T& operator*() const noexcept { return *vp; }
            const T* operator->() const noexcept { return vp; }

            pointer& operator=(T* p) noexcept {
                vp = p;
                return *this;
            }

            pointer& operator=(const T& v) noexcept {
                *vp = v;
                return *this;
            }

            explicit operator bool() const noexcept {
                return vp != nullptr;
            }

            explicit operator T* () noexcept {
                return vp;
            }

            explicit operator const T* () noexcept {
                return vp;
            }
        };

        /// @brief 示例:业务逻辑上的实体类型
        struct TestObject {
            int iV{};
            //double dV;
            pointer<double> dV;
        };
    }

    /// @brief 值类型投射示例: 这种类型可以被投射成基本类型,以减少使用时的麻烦,即,可以直接获取T类型值
    /// 注意需要投射的类型要实现以下要素
    /// 1. 继承自std::true_type;
    /// 2. type定义投射出的类型;
    /// 3. 两个to函数完成原始类型到目标类型的转换.
    template<typename T, typename U>
    struct project<examples::logic_type<T, U>> :std::true_type {
        using type = T;

        static T& to(examples::logic_type<T, U>& o) noexcept {
            return o.v;
        }

        static const T& to(const examples::logic_type<T, U>& o) noexcept {
            return o.v;
        }
    };

    /// @brief 实体类型投射示例: 这种类型可以从实体视图构造,并且能够在存储库层面提供迭代器访问,
    /// 注意需要投射的类型要实现以下要素:
    /// 1. 继承自std::true_type;
    /// 2. 定义type来指定它需要实体带哪些值类型(出于性能考虑,可以避免重复查找值类型数组)
    /// 3. 定义从type到业务类型的转换(这里没有限制,也可以让业务类型附带实体)
    template<>
    struct project<examples::TestObject> : std::true_type {
        using type = typename entity_view<int, double>;

        static void to(type& src, examples::TestObject& dst) {
            double* dV{};
            //src.project(dst.iV, dst.dV);
            src.project(dst.iV, dV);
            dst.dV = dV;
        }
    };
}
