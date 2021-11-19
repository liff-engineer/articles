#include <list>

/// @brief 消息
struct Payload {
    int iV;
    double dV;
};

/// @brief 观察者
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void Update(Payload const& msg) = 0;
};

/// @brief 对象
class Subject {
public:
    Subject() = default;
    void Attach(IObserver* ob) {
        m_observers.emplace_back(ob);
    }
    void Detach(IObserver* ob) {
        m_observers.remove(ob);
    }
    void Notify(Payload const& msg) {
        for (auto ob : m_observers) {
            ob->Update(msg);
        }
    }
private:
    std::list<IObserver*> m_observers;
};
