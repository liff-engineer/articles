#include <iostream>
#include <string>
#include "DataRef.hpp"

using namespace abc;

class MyObject {
    std::string m_str;
public:
    std::string GetStr() const {
        return m_str;
    }

    void SetStr(const std::string& v) {
        m_str = v;
    }
};

class MyObjectStrMemberRef final :public IDataRef<std::string>
{
    MyObject* m_obj;
public:
    explicit MyObjectStrMemberRef(MyObject& obj)
        :m_obj(&obj) {};

    std::string Get() const override {
        return m_obj->GetStr();
    }

    bool Set(const std::string& v) override {
        m_obj->SetStr(v);
        return true;
    }
};

constexpr auto n = sizeof(MyObjectStrMemberRef);
constexpr auto n1 = sizeof(MyObject*);

int main() {
    int iV = 10;

    DataRef<int> iObj = MakeDataRef(iV);
    std::cout << iObj.Get() << "\n";
    iObj.Set(1024);

    MyObject obj{};
    DataRef<std::string> strObj = MakeDataRef<std::string>(obj,
        std::mem_fn(&MyObject::GetStr),
        std::mem_fn(&MyObject::SetStr)
        );
    strObj.Set("liff.engineer@gmail.com");

    std::unique_ptr<IDataRef<std::string>> mRef = std::make_unique<MyObjectStrMemberRef>(obj);
    mRef->Set("liff.engineer");
    return 0;
}
