#pragma once

#include <cstdint>
#include <cstddef>

namespace prefab
{
    template <typename T>
    struct fnv1a_constant;

    template <>
    struct fnv1a_constant<std::uint32_t>
    {
        static constexpr std::uint32_t prime = 16777619;
        static constexpr std::uint32_t offset = 2166136261;
    };

    template <>
    struct fnv1a_constant<std::uint64_t>
    {
        static constexpr std::uint64_t prime = 1099511628211ull;
        static constexpr std::uint64_t offset = 14695981039346656037ull;
    };

    template <typename T, typename TChar>
    inline constexpr T fnv1a_hash(const TChar* const str, const T result = fnv1a_constant<T>::offset)
    {
        return (str[0] == '\0') ? result : fnv1a_hash(str + 1, static_cast<T>((result ^ *str) * static_cast<unsigned long long>(fnv1a_constant<T>::prime)));
    }

    template <typename T, typename TChar>
    inline constexpr T fnv1a_hash(std::size_t n, const TChar* const str, const T result = fnv1a_constant<T>::offset)
    {
        return n > 0 ? fnv1a_hash(n - 1, str + 1, (result ^ *str) * fnv1a_constant<T>::prime) : result;
    }

    template <typename T, typename TChar, std::size_t N>
    inline constexpr T fnv1a_hash(const TChar(&str)[N])
    {
        return fnv1a_hash<T>(N - 1, &str[0]);
    }

    template <typename TChar, std::size_t N>
    inline constexpr auto hash(const TChar(&str)[N])
    {
        return fnv1a_hash<std::size_t, TChar, N>(str);
    }

    template <typename T>
    inline constexpr auto hash(T const& str)
    {
        return fnv1a_hash(str.size(), str.data(), fnv1a_constant<std::size_t>::offset);
    }

    namespace detail
    {
        inline constexpr std::size_t string_literal_length(char const* s, std::size_t result = 0)
        {
            return *s == '\0' ? result : string_literal_length(s + 1, result + 1);
        }

        inline constexpr std::size_t find_left_bracket(const char* s, std::size_t result = 1) {
            return *s == '<' ? result : find_left_bracket(s + 1, result + 1);
        }

        template<std::size_t N>
        inline constexpr std::size_t find_right_bracket(const char(&s)[N], std::size_t i = N - 1, std::size_t result = 0) {
            return s[i] == '>' ? result : find_right_bracket(s, i - 1, result + 1);
        }

        struct SearchTypeWarpTag {};

        template<typename T>
        class hashed_string_literal {
        private:
            char const* data_ = nullptr;
            std::size_t size_ = 0;
            std::size_t hash_ = 0;
        public:
            constexpr hashed_string_literal() noexcept = default;
            constexpr hashed_string_literal(hashed_string_literal const&) noexcept = default;
            constexpr hashed_string_literal(char const* s) noexcept
                :data_{ s }, size_{ string_literal_length(s) }, hash_{ fnv1a_hash<std::size_t>(size_, data_) } {};

            constexpr hashed_string_literal(char const* s, std::size_t count) noexcept
                :data_{ s }, size_{ count }, hash_{ fnv1a_hash<std::size_t>(size_, data_) }{};

            template<std::size_t N>
            constexpr hashed_string_literal(const char(&s)[N]) noexcept
                :data_{ s }, size_{ N - 1 }, hash_{ fnv1a_hash<std::size_t>(size_, data_) }{};

            template<std::size_t N>
            constexpr hashed_string_literal(SearchTypeWarpTag, const char(&s)[N]) noexcept
                :data_{ s + detail::find_left_bracket(s) },
                size_{ N - 1 - detail::find_left_bracket(s) - detail::find_right_bracket(s) },
                hash_{ fnv1a_hash<std::size_t>(size_, data_) }{};

            constexpr std::size_t size()  const noexcept { return size_; };
            constexpr std::size_t length() const noexcept { return size_; };
            constexpr bool empty() const noexcept { return 0 == size_; };
            constexpr std::size_t hash() const noexcept { return hash_; };
            constexpr const char* data() const noexcept { return data_; }

            constexpr explicit operator bool() const noexcept {
                return 0 != size_;
            }

            constexpr bool operator<(const hashed_string_literal<T>& other) const noexcept {
                return hash_ < other.hash();
            }
        };
    }


    using HashedStringLiteral = detail::hashed_string_literal<char>;

    template<typename T>
    constexpr bool operator==(detail::hashed_string_literal<T> const& lhs, detail::hashed_string_literal<T> const& rhs) noexcept {
        return lhs.hash() == rhs.hash();
    }

    template<typename T>
    constexpr bool operator!=(detail::hashed_string_literal<T> const& lhs, detail::hashed_string_literal<T> const& rhs) noexcept {
        return lhs.hash() != rhs.hash();
    }

    template<typename T>
    constexpr bool operator<(detail::hashed_string_literal<T> const& lhs, detail::hashed_string_literal<T> const& rhs) noexcept {
        return lhs.hash() < rhs.hash();
    }

    constexpr bool operator<(HashedStringLiteral const& lhs, HashedStringLiteral const& rhs) {
        return lhs.hash() < rhs.hash();
    }

    template<typename T>
    constexpr bool operator<=(detail::hashed_string_literal<T> const& lhs, detail::hashed_string_literal<T> const& rhs) noexcept {
        return lhs.hash() <= rhs.hash();
    }

    template<typename T>
    constexpr bool operator>(detail::hashed_string_literal<T> const& lhs, detail::hashed_string_literal<T> const& rhs) noexcept {
        return lhs.hash() > rhs.hash();
    }

    template<typename T>
    constexpr bool operator>=(detail::hashed_string_literal<T> const& lhs, detail::hashed_string_literal<T> const& rhs) noexcept {
        return lhs.hash() >= rhs.hash();
    }

    inline namespace literals
    {
        constexpr HashedStringLiteral operator "" _hashed(const char* str, std::size_t len) noexcept {
            return HashedStringLiteral(str,len);
        }
    }

    template<typename T>
    constexpr prefab::HashedStringLiteral HashedTypeNameOf() {
        //class prefab::HashedStringLiteral __cdecl prefab::HashedTypeNameOf<int>(void)
        //return { __FUNCSIG__,sizeof("class prefab::HashedStringLiteral __cdecl prefab::HashedTypeNameOf<")-1,sizeof(">(void)")-1 };
        return { detail::SearchTypeWarpTag{},__FUNCSIG__ };
    }

    template<typename T>
    struct HashedTypeName
    {
        static constexpr auto value = HashedTypeNameOf<T>();
    };

    template<typename T>
    constexpr auto HashedTypeName_v = HashedTypeName<T>::value;

} // namespace prefab
