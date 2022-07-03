#include "PyPlugin.h"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <Windows.h>

namespace py = pybind11;

void ReportError(const std::runtime_error& e,const char* message) {
    OutputDebugStringA(e.what());
    ::MessageBoxA(NULL, e.what(), message, MB_OK);
}

class Plugin {
public:
    Plugin()
    {
        try {
            m = py::module::import("Test");
            m.attr("initialize").call();
        }
        catch (const std::runtime_error& e) {
            ReportError(e, "Initialize Test.py Failed");
            throw e;
        }
    }
private:
    py::module m;
};

static Plugin* plugin = nullptr;

void Initialize()
{
    py::initialize_interpreter();
    try {
        py::module::import("PyActor");
    }
    catch (const std::runtime_error& e) {
        ReportError(e, "PyPlugin Initialize failed");
        py::finalize_interpreter();
        return;
    }

    plugin = new Plugin();
}

void Finalize()
{
    if (plugin) {
        delete plugin;
        py::finalize_interpreter();
    }
}
