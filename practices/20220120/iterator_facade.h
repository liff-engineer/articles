#pragma once

#include <type_traits>
#include <iterator>

namespace abc
{
    template<
        typename T,
        typename IteratorCategory,
        typename ValueType,
        typename Reference = ValueType&,
        typename Pointer = ValueType*,
        typename DifferenceType = std::ptrdiff_t,
        typename E = std::enable_if_t<
        std::is_class_v<T>&&
        std::is_same_v<T, std::remove_cv_t<T>>
        >
    >
        struct iterator_facade;

    namespace detail {
        template<typename Iterator>
        constexpr auto is_ra_iter = std::is_base_of_v<std::random_access_iterator_tag,typename Iterator::iterator_category>;

        template<typename T, typename IteratorCategory, typename ValueType, typename Reference, typename Pointer, typename DifferenceType>
        void is_facade(iterator_facade<T, IteratorCategory, ValueType, Reference, Pointer, DifferenceType> const&);

        template<typename Iterator>
        auto distance(Iterator const& lhs, Iterator const& rhs) noexcept(noexcept(lhs.distance_to(rhs)))
            ->decltype(lhs.distance_to(rhs))
        {
            return lhs.distance_to(rhs);
        }
    }

    template<typename T, typename IteratorCategory, typename ValueType, typename Reference, typename Pointer, typename DifferenceType, typename E >
    struct iterator_facade {
    public:
        using difference_type = DifferenceType;
        using value_type = ValueType;
        using pointer = std::conditional_t<std::is_same<IteratorCategory, std::output_iterator_tag>::value, void, Pointer>;
        using reference = Reference;
        using iterator_category = IteratorCategory;

        template<typename U = T>
        U& operator++() noexcept(noexcept(std::declval<U&>().advance(1)))
        {
            return static_cast<U&>(*this).advance(1);
        }

        template<typename U = T>
        U& operator--() noexcept(noexcept(std::declval<U&>().advance(-1)))
        {
            return static_cast<U&>(*this).advance(-1);
        }

        template<typename U = T>
        U operator++(int) noexcept(noexcept(std::declval<U&>().advance(1)))
        {
            U result = static_cast<U&>(*this);
            static_cast<U&>(*this).advance(1);
            return result;
        }

        template<typename U = T>
        U operator--(int) noexcept(noexcept(std::declval<U&>().advance(-1)))
        {
            U result = static_cast<U&>(*this);
            static_cast<U&>(*this).advance(-1);
            return result;
        }

        template<typename U = T>
        U& operator+=(difference_type n) noexcept(noexcept(std::declval<U&>().advance(n)))
        {
            return static_cast<U&>(*this).advance(n);
        }

        template<typename U = T>
        U& operator-=(difference_type n) noexcept(noexcept(std::declval<U&>().advance(-n)))
        {
            return static_cast<U&>(*this).advance(-n);
        }

        template<typename U = T>
        U operator+(difference_type n) const noexcept(noexcept(std::declval<U&>().advance(n)))
        {
            U result = static_cast<const U&>(*this);
            result.advance(n);
            return result;
        }

        template<typename U = T>
        U operator-(difference_type n) const noexcept(noexcept(std::declval<U&>().advance(-n)))
        {
            U result = static_cast<const U&>(*this);
            result.advance(-n);
            return result;
        }

        friend T operator+(difference_type n, const T& it) {
            return it + n;
        }

        template<typename U = T>
        auto operator-(const U& other) const noexcept(noexcept(std::declval<U&>().distance_to(other)))
        {
            return static_cast<const U&>(*this).distance_to(other);
        }
    };

    template<typename Iter, typename E = std::enable_if_t<detail::is_ra_iter<Iter>>>
    auto operator==(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) == 0) {
        return detail::distance(lhs, rhs) == 0;
    }

    template<typename Iter>
    auto operator!=(const Iter& lhs, const Iter& rhs) noexcept(noexcept(!(lhs == rhs)))
        ->decltype(detail::is_facade(lhs), !(lhs == rhs)) {
        return !(lhs == rhs);
    }

    template<typename Iter>
    auto operator<(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) < 0) {
        return detail::distance(lhs, rhs) < 0;
    }

    template<typename Iter>
    auto operator<=(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) <= 0) {
        return detail::distance(lhs, rhs) <= 0;
    }

    template<typename Iter>
    auto operator>(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) > 0) {
        return detail::distance(lhs, rhs) > 0;
    }

    template<typename Iter>
    auto operator>=(const Iter& lhs, const Iter& rhs) noexcept(noexcept(detail::distance(lhs, rhs)))
        ->decltype(detail::is_facade(lhs), detail::distance(lhs, rhs) >= 0) {
        return detail::distance(lhs, rhs) >= 0;
    }
}
