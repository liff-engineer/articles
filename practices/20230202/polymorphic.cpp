#include "polymorphic_value.h"
#include <iostream>

using namespace isocpp_p0201;

namespace {
    struct BaseType {
        virtual int value() const = 0;
        virtual void set_value(int) = 0;
        virtual ~BaseType() = default;
    };

    struct DerivedType : BaseType {
        int value_ = 0;

        DerivedType() { ++object_count; }

        DerivedType(const DerivedType& d) {
            value_ = d.value_;
            ++object_count;
        }

        DerivedType(int v) : value_(v) { ++object_count; }

        ~DerivedType() { --object_count; }

        int value() const override { return value_; }

        void set_value(int i) override { value_ = i; }

        static size_t object_count;
    };

    size_t DerivedType::object_count = 0;

}  // namespace

int main() {
    {
        class Incomplete;
        polymorphic_value<Incomplete> p;
        if (!p) {
            std::cout << "polymorphic_value<Incomplete>\n";
        }
    }
    {
        DerivedType d{ 7 };
        polymorphic_value<BaseType> i(std::in_place_type<DerivedType>, d);
        if (i->value() == 7) {
            std::cout << "polymorphic_value<BaseType> inplace copy ctor\n";
        }
    }
    {
        int v = 7;
        polymorphic_value<BaseType> original_cptr(std::in_place_type<DerivedType>, v);
        polymorphic_value<BaseType> cptr(original_cptr);
    }
    {

    }

    return 0;
}
