#include <iostream>
#include "chain.hpp"

class Base {
public:
    virtual ~Base() = default;
};

class OtherClass {
public:
    int GetIntger() const {
        return 1024;
    }
};

class DerivedClass :public Base
{
    OtherClass b{};
public:
    const OtherClass* GetData() const  noexcept {
        //return &b;
        return nullptr;
    }

    OtherClass* GetData()  noexcept {
        return &b;
        //return nullptr;
    }
};

struct GetData {
    OtherClass* op(DerivedClass* obj) {
        if (obj) {
            return obj->GetData();
        }
        std::cout << "empty instance\n";
        return {};
    }

    const OtherClass* op(const DerivedClass* obj) {
        if (obj) {
            return obj->GetData();
        }
        std::cout << "empty instance\n";
        return {};
    }

    OtherClass* op(Base* obj) {
        return op(dynamic_cast<DerivedClass*>(obj));
    }

    const OtherClass* op(const Base* obj) {
        return op(dynamic_cast<const DerivedClass*>(obj));
    }
};

struct GetIntger {
    int op(OtherClass const* obj) {
        if (obj) {
            return obj->GetIntger();
        }
        std::cout << "empty instance;use default value \n";
        return {};
    }

    int op(Base* obj) {
        return op(abc::chain(obj, GetData{}));
    }

    int op(const Base* obj) {
        return op(abc::chain(obj, GetData{}));
    }
};


//可以尝试修改为const版本
void example(Base* base) {
    using namespace abc;

    auto v2 = chain(base, 
        cast<const DerivedClass>{}, 
        GetData{},
        GetIntger{});

    //auto v5 = chain(base, GetData{});
    //auto v2 = chain(base, GetIntger{});
    //常规写法
    // int v3{};
    // auto o = dynamic_cast<const DerivedClass*>(base);
    // if (o) {
    // 	auto o1 = o->GetData();
    // 	if (o1) {
    // 		v3 = o1->GetIntger();
    // 	}
    // }

    std::cout << "example:" << v2 << "\n";
}

int main(int argc, char** argv) {
    DerivedClass obj{};
    example(&obj);
    return 0;
}
