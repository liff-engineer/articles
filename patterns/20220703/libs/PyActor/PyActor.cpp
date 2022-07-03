#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "Actor.h"

namespace py = pybind11;

/// @brief 接口类需要提供跳板
class PyActor :public abc::IActor {
public:
    using abc::IActor::IActor;

    ~PyActor() noexcept {
        //py::print("~PyActor");
    }
    void Launch() override {
        PYBIND11_OVERLOAD_PURE(
            void,
            abc::IActor,
            Launch
        )
    }
};

//https://github.com/pybind/pybind11/issues/1902
class PyActorWrapper :public abc::IActor {
public:
    PyActorWrapper(py::object actor)
        :m_actor(std::move(actor)) {};
    virtual ~PyActorWrapper() = default;
    void Launch() override {
        m_actor.attr("Launch")();
    }
private:
    py::object m_actor;
};

void  Test(abc::IActor* actor) {
    if(actor) {
        actor->Launch();
    }
}

PYBIND11_MODULE(PyActor, m) {
    m.doc() = "Actor Python Api Module";

    //注意返回值策略,python端只能用,不能进行操作
    m.def("gFactory", &abc::gActorFactory,py::return_value_policy::reference);
    m.def("Test", Test);

    py::class_<abc::IActor, PyActor, std::shared_ptr<abc::IActor>>(m, "IActor")
        .def(py::init<>())
        .def("Launch", &abc::IActor::Launch);

    py::class_<abc::ActorFactory>(m, "Factory")
        .def("Codes", &abc::ActorFactory::Codes)
        .def("Make", [](abc::ActorFactory& obj, const std::string& code)->std::shared_ptr<abc::IActor> {
        return obj.Make(code);
            })
        .def("Register", [](abc::ActorFactory& obj,
            const std::string& code, py::object actorClass) ->bool {
                return obj.Register(code,
                    [op = std::move(actorClass)]()->std::unique_ptr<abc::IActor>{
                    return std::make_unique<PyActorWrapper>(op());
                });
            })
        ;
}
