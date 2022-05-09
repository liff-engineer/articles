#pragma once
#include <vector>
#include <memory>
#include <iterator>

namespace abc
{
    inline namespace v1
    {
        namespace im
        {
            class IValueArray {
            public:
                virtual ~IValueArray() = default;
                virtual void erase(std::size_t index) noexcept = 0;
                virtual std::unique_ptr<IValueArray> clone() const = 0;
            };

            template<typename T>
            class ValueArray final :public IValueArray
            {
                static_assert(!std::is_same_v<T, bool>, "cann't support because std::vector<bool>");
            public:
                explicit ValueArray(std::size_t capacity)
                {
                    auto block = std::make_unique<std::vector<T>>();
                    block->reserve(capacity);
                    m_blocks.emplace_back(std::move(block));
                    m_pointers.reserve(capacity);
                }

                std::size_t size() const noexcept { return m_pointers.size(); }

                bool empty() const noexcept {
                    return std::all_of(m_pointers.begin(), m_pointers.end(), [](auto v) { return v == nullptr; });
                };

                bool contains(std::size_t index) const noexcept {
                    if (index >= m_pointers.size()) return false;
                    return m_pointers[index] != nullptr;
                }

                template<typename... Args>
                void emplace(std::size_t index, Args&&... args) {
                    if (index + 1 >= m_pointers.size()) {
                        m_pointers.resize(index + 1, nullptr);
                    }

                    auto&& pointer = m_pointers[index];
                    if (pointer == nullptr) {
                        pointer = alloc(std::forward<Args>(args)...);
                    }
                    else {
                        *pointer = T{ std::forward<Args>(args)... };
                    }
                }

                std::add_pointer_t<T> at(std::size_t i) {
                    if (i < m_pointers.size()) return m_pointers.at(i);
                    return nullptr;
                }

                std::add_pointer_t<T> operator[](std::size_t i) noexcept { return m_pointers[i]; }

                void erase(std::size_t index) noexcept override {
                    if (index < m_pointers.size()) {
                        auto&& pointer = m_pointers[index];
                        if (pointer != nullptr) {
                            std::exchange(*pointer, T{});
                            pointer = nullptr;
                        }
                    }
                }

                std::unique_ptr<IValueArray> clone() const override {
                    //clone时计算出合适的大小,避免重复内存申请
                    std::size_t n = 0;
                    for (std::size_t i = 0; i < m_pointers.size(); i++) {
                        if (m_pointers[i] != nullptr) {
                            n++;
                        }
                    }
                    auto result = std::make_unique<ValueArray<T>>(n);

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
            protected:
                template<typename... Args>
                std::add_pointer_t<T> alloc(Args&&... args) {
                    auto  current = m_blocks.back().get();
                    const auto capacity = current->capacity();
                    const auto size = current->size();
                    if (size + 1 > capacity) {
                        auto block = std::make_unique<std::vector<T>>();
                        //注意空间分配不要太小
                        block->reserve(capacity > 4 ? capacity * 2 : 8);
                        m_blocks.emplace_back(std::move(block));
                        current = m_blocks.back().get();
                    }
                    current->emplace_back(std::forward<Args>(args)...);
                    return std::addressof(current->back());
                }
            private:
                std::vector<std::unique_ptr<std::vector<T>>> m_blocks;
                std::vector<std::add_pointer_t<T>> m_pointers;
            };

            template<typename T>
            class ValueArrayProxy final {
            public:
                template<typename T>
                class Item {
                    int m_key{ -1 };
                    T* m_value{};
                public:
                    Item() = default;
                    explicit Item(int key, T* vp) :m_key(key), m_value(vp) {};

                    int  key() const noexcept { return m_key; }
                    const T& value()  const noexcept { return *m_value; };
                    T& value() noexcept { return *m_value; };
                };

                template<typename T>
                struct Iterable
                {
                    using difference_type = std::ptrdiff_t;
                    using value_type = Item<T>;
                    using pointer = value_type*;
                    using reference = value_type&;
                    using iterator_category = std::forward_iterator_tag;

                    explicit Iterable(ValueArray<T>* array_arg, int i)
                        :array(array_arg), value(i, nullptr) {
                        if (array && value.key() < array->size()) {
                            value = Item<T>{ value.key(),(*array)[value.key()] };
                        }
                    };

                    bool operator==(const Iterable& rhs) const noexcept {
                        return (array == rhs.array) && (value.key() == rhs.value.key());
                    }

                    bool operator!=(const Iterable& rhs) const noexcept {
                        return !(*this == rhs);
                    }

                    void swap(Iterable& other) {
                        using std::swap;
                        swap(array, other.array);
                        swap(value, other.value);
                    }

                    reference operator*() noexcept {
                        return value;
                    }
                    pointer  operator->() noexcept {
                        return std::addressof(value);
                    }

                    Iterable& operator++() noexcept { forward(); return *this; };

                    Iterable operator++(int) noexcept {
                        Iterable ret = *this;
                        ++* this;
                        return ret;
                    }
                private:
                    void forward() noexcept {
                        if (!array) return;
                        auto n = array->size();
                        for (auto i = value.key() + 1; i < n; i++) {
                            if ((*array)[i] != nullptr) {
                                value = Item<T>{ int(i),(*array)[i] };
                                return;
                            }
                        }
                        value = Item<T>{ int(n),nullptr };
                    }
                private:
                    ValueArray<T>* array;
                    Item<T>        value;
                };
            public:
                explicit ValueArrayProxy(ValueArray<T>* array)
                    :m_array(array) {};

                auto begin() noexcept {
                    return Iterable{ m_array,0 };
                }

                auto end() noexcept {
                    return Iterable{ m_array,
                            m_array != nullptr ? (int)(m_array->size()) : 0
                    };
                }
            private:
                ValueArray<T>* m_array{ nullptr };
            };
        }
    }
}
