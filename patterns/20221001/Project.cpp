//使用以下库
//plf::hive https://github.com/mattreecebentley/plf_hive
//jss::ticket_map https://github.com/anthonywilliams/ticketmap

#include <iostream>
#include <string>

#include "ticket_map.hpp"
#include "plf_hive.h"

void test_ticket_map()
{
    jss::ticket_map<int, std::string> map;
    auto ticket1 = map.insert("hello");
    auto ticket2 = map.insert("world");

    std::cout << map[ticket1] << " " << map[ticket2] << std::endl;

    map.erase(ticket1);
}

void test_plf_hive()
{
    plf::hive<int> i_hive;
    plf::hive<int>::iterator it;
    plf::hive<int*> p_hive;
    plf::hive<int*>::iterator p_it;
    std::vector<int*> intptrs;

    // Insert 100 ints to i_hive and pointers to those ints to p_hive:
    for (int i = 0; i != 100; ++i)
    {
        it = i_hive.insert(i);
        p_hive.insert(&(*it));
        intptrs.push_back(&(*it));
    }

    // Erase half of the ints:
    for (it = i_hive.begin(); it != i_hive.end(); ++it)
    {
        it = i_hive.erase(it);
    }

    // Erase half of the int pointers:
    for (p_it = p_hive.begin(); p_it != p_hive.end(); ++p_it)
    {
        p_it = p_hive.erase(p_it);
    }

    // Total the remaining ints via the pointer hive (pointers will still be valid even after insertions and erasures):
    int total = 0;

    for (p_it = p_hive.begin(); p_it != p_hive.end(); ++p_it)
    {
        total += *(*p_it);
    }

    std::cout << "Total: " << total << std::endl;

    if (total == 2500)
    {
        std::cout << "Pointers still valid!" << std::endl;
    }
}

#include "DataSet.h"
#include "DataSetCluster.hpp"

void test_dsc()
{
    using namespace abcd;

}
int main() {
    {
        abc::test_project();
        return 0;
    }
    plf::hive<int> values;
    values.insert(10);
    auto it = values.insert(20);
    int* vp = &(*it);
    auto n1 = values.size();
    values.erase(values.get_iterator(vp));
    auto n2 = values.size();
    test_ticket_map();
    test_plf_hive();
    return 0;
}
