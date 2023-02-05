#pragma once

#include "ImplValue.h"
#include <string>

class IPrinter {
public:
    virtual ~IPrinter() = default;
    virtual void print(const std::string& msg) const = 0;
};

class PrinterImpl;
class Printer :public IPrinter {
public:
    Printer();
    Printer(std::string name);
    void print(const std::string& msg) const override;
private:
    std::string m_name{};
    abc::ImplValue<PrinterImpl> m_impl{};
};
