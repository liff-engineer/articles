#pragma once
#include <type_traits>

namespace abc {
    namespace detail {
        template<typename T>
        struct member_helper : std::false_type {};

        template<typename T, typename R>
        struct member_helper<R T::*> : std::is_member_object_pointer<R T::*> {
            using type = T;
            using member_type = R;
        };
    }

    template<typename T, typename R, R T::* M>
    struct member {
        static constexpr auto mp = M;
        const char* name;
        std::size_t offset;
    };

#ifdef __cpp_nontype_template_parameter_auto
    template<auto m>
    constexpr auto make_member(const char* name, std::size_t offset) {
        using helper = detail::member_helper<decltype(m)>;
        static_assert(helper::value, "is't member object pointer");
        return member<helper::type, helper::member_type, m>{name, offset};
    }
#else
    template<typename M, M m>
    constexpr auto make_member(const char* name, std::size_t offset) {
        using helper = detail::member_helper<M>;
        static_assert(helper::value, "is't member object pointer");
        return member<helper::type, helper::member_type, m>{name, offset};
    }
#endif

    template<typename T, typename E = void>
    struct members :std::false_type {};
}

#ifdef __cpp_nontype_template_parameter_auto
#define ABC_MAKE_MEMBER(s,m)   abc::make_member<&s::m>(#m,offsetof(s,m))
#else
#define ABC_MAKE_MEMBER(s,m)   abc::make_member<decltype(&s::m),&s::m>(#m,offsetof(s,m))
#endif
