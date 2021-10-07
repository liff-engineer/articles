#pragma once
#include <string>
#include <vector>

struct Object
{
    bool bV;
    int iV;
    double dV;
    std::string sV;
    std::vector<int> iVs;
};

namespace abc {
    enum class Flag
    {
        F1,
        F2,
        F3
    };
}

