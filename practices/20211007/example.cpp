#include "members.hpp"
#include "print.hpp"
#include "example.hpp"
#include "example.members.hpp"

#if 0
template <>
struct abc::members<Object> : std::true_type
{
    template <typename Op>
    static void for_each(Op &op)
    {
        op(ABC_MAKE_MEMBER(Object, bV));
        op(ABC_MAKE_MEMBER(Object, iV));
        op(ABC_MAKE_MEMBER(Object, dV));
        op(ABC_MAKE_MEMBER(Object, sV));
        op(ABC_MAKE_MEMBER(Object, iVs));
    }
};


template <>
struct abc::members<Flag> : std::true_type
{
    template <typename Op>
    static void for_each(Op &op)
    {
        op(Flag::F1, "F1");
        op(Flag::F2, "F2");
        op(Flag::F3, "F3");
    }
};
#endif

std::ostream &operator<<(std::ostream &os, std::vector<int> const &vs)
{
    os << "(";
    for (std::size_t i = 0; i < vs.size(); i++)
    {
        os << vs[i];
        if (i != vs.size() - 1)
        {
            os << ",";
        }
    }
    os << ")";
    return os;
}

int main(int argc, char **argv)
{
    print_struct_info<Object>();
    print_enum_info<abc::Flag>();

    Object o{};
    o.bV = true;
    o.iV = 256;
    o.dV = 1.414;
    o.sV = "liff.engineer@gmail.com";
    o.iVs = {1, 3, 5, 6};

    print(o);
    print(o.sV);
    return 0;
}