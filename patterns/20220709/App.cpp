#include "Observer.hpp"
#include <iostream>
#include <string>

using Event = v0::Event;
namespace v0
{
    class SubjectObserver : public IObserver
    {
    public:
        void update(double event) override
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
    };

    class EventObserver : public IEventObserver
    {
    public:
        void update(const Event &event) override
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };

    void test()
    {
        Subject obj1{};
        SubjectObserver ob1{};
        EventSubject obj2{};
        EventObserver ob2{};

        obj1.subscribe(&ob1);
        obj2.subscribe(&ob2);

        obj1.notify(3.1415926);
        obj2.notify(Event{"message coming"});
    }
}

namespace v1
{
    class SubjectObserver : public IObserver<double>
    {
    public:
        void update(const double &event) override
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
    };

    class EventObserver : public IObserver<Event>
    {
    public:
        void update(const Event &event) override
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };

    void test()
    {
        Subject<double> obj1{};
        SubjectObserver ob1{};
        Subject<Event> obj2{};
        EventObserver ob2{};

        obj1.subscribe(&ob1);
        obj2.subscribe(&ob2);

        obj1.notify(3.1415926);
        obj2.notify(Event{"message coming"});
    }
}

namespace v2
{
    class SubjectObserver : public Observer<double>
    {
    public:
        void update(const double &event) override
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
    };

    class EventObserver : public Observer<Event>
    {
    public:
        void update(const Event &event) override
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };

    void test()
    {
        Publisher publisher{};
        SubjectObserver ob1{};
        EventObserver ob2{};

        publisher.subscribe(&ob1);
        publisher.subscribe(&ob2);

        publisher.notify(3.1415926);
        publisher.notify(Event{"message coming"});
    }
}

namespace v3
{
    class MyObserver
    {
    public:
        void update(double event)
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
        void update(const Event &event)
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };

    void test()
    {
        Publisher publisher{};
        MyObserver ob{};

        publisher.subscribe<double>(&ob);
        publisher.subscribe<Event>(&ob);

        publisher.notify(3.1415926);
        publisher.notify(Event{"message coming"});
    }
}

namespace v4
{
    class MyObserver
    {
    public:
        void update(double event)
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
        void update(const Event &event)
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };
    void test()
    {
        Publisher publisher{};
        MyObserver ob{};

        publisher.subscribe<double>(&ob);
        publisher.subscribe<Event>(&ob);

        publisher.notify(3.1415926);
        publisher.notify(Event{"message coming"});
    }
}

namespace v5
{
    class MyObserver
    {
    public:
        void update(double event)
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
        void update(const Event &event)
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };

    void test()
    {
        Publisher publisher{};
        MyObserver ob{};

        auto stub1 = publisher.subscribe<double>(&ob);
        publisher.subscribe<Event>(&ob);

        publisher.notify(3.1415926);
        publisher.notify(Event{"message coming"});
    }
}

namespace v6
{
    class MyObserver
    {
    public:
        void update(double event)
        {
            std::cout << __FUNCSIG__ << ":" << event << '\n';
        }
        void update(const Event &event)
        {
            std::cout << __FUNCSIG__ << ":" << event.payload << '\n';
        }
    };

    void test()
    {
        Publisher publisher{};
        MyObserver ob{};

        publisher.subscribe<double>(&ob);
        publisher.subscribe<Event>(&ob);
        publisher.subscribe<Event>([](const Event &e)
                                   { std::cout << __FUNCSIG__ << ":" << e.payload << '\n'; });

        publisher.notify(3.1415926);
        publisher.notify(Event{"message coming"});
    }
}

int main()
{
    v0::test();
    v1::test();
    v2::test();
    v3::test();
    v4::test();
    v5::test();
    v6::test();
    return 0;
}
