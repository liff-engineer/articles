#include "archive.h"

struct MyObject
{
    bool bV;
    int iV;
    double dV;
    std::string sV;
    std::vector<int> iVs;
    std::map<int, std::string> kVs;
};

struct MyComplexObject
{
    MyObject oV;
    std::vector<double> dVs;
};

void ArWrite(IArchive* ar, const MyObject& obj) {
    ar->write("bV", obj.bV);
    ar->write("iV", obj.iV);
    ar->write("dV", obj.dV);
    ar->write("sV", obj.sV);
    ar->write("iVs", obj.iVs);
    ar->write("kVs", obj.kVs);
}

bool ArRead(const IArReader* ar, MyObject& obj) {
    ar->read("bV", obj.bV);
    ar->read("iV", obj.iV);
    ar->read("dV", obj.dV);
    ar->read("sV", obj.sV);
    ar->read("iVs", obj.iVs);
    ar->read("kVs", obj.kVs);
    return true;
}

void ArWrite(IArchive* ar, const MyComplexObject& obj) {
    ar->write("oV", obj.oV);
    ar->write("dVs", obj.dVs);
}

bool ArRead(const IArReader* ar, MyComplexObject& obj) {
    ar->read("oV", obj.oV);
    ar->read("dVs", obj.dVs);
    return true;
}

void test_load_json(std::string const& file) {
    auto ar = Archive("json");
    ar->open(file);

    MyObject obj{};
    ar->read("MyObject", obj);
    MyComplexObject obj1{};
    ar->read("MyComplexObject", obj1);

    if (obj1.dVs.empty()) {

    }
}


void test_load_msgpack(std::string const& file) {
    auto ar = Archive("MessagePack");
    ar->open(file);

    MyObject obj{};
    ar->read("MyObject", obj);
    MyComplexObject obj1{};
    ar->read("MyComplexObject", obj1);

    if (obj1.dVs.empty()) {

    }
}

void test_archive_clone(const Archive& other)
{
    Archive ar{ other };

    MyObject obj{};
    ar->read("MyObject", obj);
    MyComplexObject obj1{};
    ar->read("MyComplexObject", obj1);
    if (obj1.dVs.empty()) {

    }
}

int main() {
    {
        auto ar = Archive("json");
        ar->write("bV", true);
        ar->write("iV", 1024);
        ar->write("dV", 3.1415926);
        ar->write("sV", "liff.engineer@gmail.com");
        ar->write("iVs", std::vector<int>{2, 4, 6, 8, 10});
        auto result = ar->to_string();
        ar->save("archive.json");
    }

    MyObject obj{};
    obj.bV = true;
    obj.iV = 1024;
    obj.dV = 3.1415926;
    obj.sV = "liff.engineer@gmail.com";
    obj.iVs = { 1,3,5,7,9 };
    obj.kVs = { {1,"liff.xian@gmail.com"},{2,"liff.cpplang@gmail.com"} };

    MyComplexObject obj1{};
    obj1.oV = obj;
    obj1.dVs = { 1.1,2.2,3.3,4.4 };

    auto ar = Archive("json");
    ar->write("MyObject", obj);
    ar->write("MyComplexObject", obj1);
    {
        auto ar1 = Archive("json");
        ar1->write_as(obj);
        ar->write("Clone", ar1);
    }
    auto result = ar->to_string();
    ar->save("result.json");
    test_load_json("result.json");

    test_archive_clone(ar);

    auto ar1 = Archive("MessagePack");
    ar1->write("MyObject", obj);
    ar1->write("MyComplexObject", obj1);
    ar1->save("result.bin");
    test_load_msgpack("result.bin");
    return 0;
}
