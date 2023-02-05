#include "Printer.h"
#include <vector>

int main() {
    Printer printer{"Printer"};
    printer.print("liff.engineer@gmail.com");

    Printer clone = printer;
    clone.print("2023/2/5");

    std::vector<Printer> values{};
    values.emplace_back("P1");
    values.emplace_back("P2");
    values.emplace_back("P3");
    for (auto& e : values) {
        e.print("2:07");
    }
    return 0;
}
