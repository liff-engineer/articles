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
    //ar->write("iVs", obj.iVs);
    //ar->write("kVs", obj.kVs);
}

bool ArRead(const IArReader* ar, MyObject& obj) {
    ar->read("bV", obj.bV);
    ar->read("iV", obj.iV);
    ar->read("dV", obj.dV);
    ar->read("sV", obj.sV);
    //ar->read("iVs", obj.iVs);
    //ar->read("kVs", obj.kVs);
    return true;
}

void ArWrite(IArchive* ar, const MyComplexObject& obj) {
    ar->write("oV", obj.oV);
    //ar->write("dVs", obj.dVs);
}

bool ArRead(const IArReader* ar, MyComplexObject& obj) {
    ar->read("oV", obj.oV);
    //ar->read("dVs", obj.dVs);
    return true;
}

void test_load_xml(std::string const& file) {
    auto ar = Archive("xml");
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

    MyObject obj{};
    obj.bV = false;
    obj.iV = 1024;
    obj.dV = 3.1415926;
    obj.sV = "liff.engineer@gmail.com";
    obj.iVs = { 1,3,5,7,9 };
    obj.kVs = { {1,"liff.xian@gmail.com"},{2,"liff.cpplang@gmail.com"} };
    
    MyComplexObject obj1{};
    obj1.oV = obj;
    obj1.dVs = { 1.1,2.2,3.3,4.4 };

    auto ar = Archive("xml");
    ar->write_tag("Document");
    ar->write("MyComplexObject", obj1);
    ar->write("MyObject", obj);
    {
        auto ar1 = Archive("xml");
#if 0
        ar1->write_tag("Clone");
        ar1->write("MyComplexObject", obj1);
        ar1->write("MyObject", obj);
#else
        ar1->write_tag("MyObjectClone");
        ar1->write_as(obj);
#endif
        ar->write(ar1);
    }
    auto result = ar->to_string();
    ar->save("archive.xml");

    test_archive_clone(ar);

    test_load_xml("archive.xml");
    return 0;
}
