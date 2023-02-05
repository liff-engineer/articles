#include "DynValue.h"
#include <iostream>
#include <vector>
#include <set>
#include <unordered_map>

using namespace abc;

class ITask {
public:
    virtual ~ITask() = default;
    virtual void run() const noexcept = 0;
    virtual std::unique_ptr<ITask> clone() const = 0;
    virtual bool equal(const ITask* other) const = 0;
    virtual bool less(const ITask* other) const = 0;
};

class PrintTask :public ITask {
    std::string msg;
public:
    explicit PrintTask(std::string v) :msg(std::move(v)) {};

    void run() const noexcept override {
        std::cout << msg << "\n";
    }

    std::unique_ptr<ITask> clone() const override {
        return std::make_unique<PrintTask>(*this);
    }

    bool equal(const ITask* other) const override {
        if (auto o = dynamic_cast<const PrintTask*>(other)) {
            return o->msg == msg;
        }
        return false;
    }

    bool less(const ITask* other) const override {
        if (auto o = dynamic_cast<const PrintTask*>(other)) {
            return o->msg < msg;
        }
        return false;
    }
};

class MyPrintTask :public PrintTask {
public:
    using PrintTask::PrintTask;

    void run() const noexcept override {
        std::cout << "My:";
        PrintTask::run();
    }

    std::unique_ptr<ITask> clone() const override {
        return std::make_unique<MyPrintTask>(*this);
    }
};

int main() {
    DynValue<ITask> v{};
    v = MakeDynValue<ITask,PrintTask>("liff.engineer@gmail.com");
    v->run();
    DynValue<ITask> v_clone = v;
    v_clone->run();

    DynValue<ITask> v1{ std::make_unique<PrintTask>("PrintTask to ITask")};
    v1->run();

    MyPrintTask task{ "2023/2/3" };
    DynValue<PrintTask> v2{ &task };
    v2->run();

    DynValue<ITask> v3{ v2 };
    v3->run();

    std::unique_ptr<PrintTask> v4 = std::move(v2);
    v4->run();

    std::vector<DynValue<ITask>> tasks{};
    tasks.emplace_back(std::make_unique<PrintTask>("T1"));
    tasks.emplace_back(std::make_unique<PrintTask>("T2"));
    tasks.emplace_back(std::make_unique<PrintTask>("T3"));

    std::vector<DynValue<ITask>> tasks_clone = tasks;

    PrintTask task1{ "T1" };
    auto it = std::find(std::begin(tasks), std::end(tasks), &task1);
    if (it != std::end(tasks)) {
        (*it)->run();
    }

    std::set<DynValue<ITask>> set{};
    set.insert(tasks.at(0));
    set.insert(tasks.at(0));

    return 0;
}
