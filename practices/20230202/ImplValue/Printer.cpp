#include "Printer.h"
#include <iostream>

class PrinterImpl {
public:
    void print(const std::string& name, const std::string& msg) const {
        std::cout << name << ":" << msg << "\n";
    }
private:
    int iV{ 1024 };
};

Printer::Printer()
    :Printer(std::string{})
{
}

Printer::Printer(std::string name)
    :m_name(std::move(name)), m_impl{ abc::MakeImplValue<PrinterImpl>()}
{
}

void Printer::print(const std::string& msg) const
{
    m_impl->print(m_name, msg);
}
