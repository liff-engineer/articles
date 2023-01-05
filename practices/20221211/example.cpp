#include <iostream>

template<typename T>
class IStateVisitor;

template<typename T>
class IState;

class IVisitable {
public:
    virtual ~IVisitable() = default;
};

class IVisitor {
public:
    virtual ~IVisitor() = default;
};

template<typename T>
class IStateVisitor :public IVisitor {
public:
    virtual void visit(T& object) = 0;
};

template<typename T>
class IState : public IVisitable {
public:
    virtual void accept(IStateVisitor<T>& visiter) = 0;
};

template<typename T>
class State final :public IState<T> {
    T m_obj;
public:
    template<typename... Args>
    explicit State(Args&&... args)
        :m_obj{ std::forward<Args>(args)... } {};

    void accept(IStateVisitor<T>& visitor) override {
        visitor.visit(m_obj);
    }
};

template<typename T>
class StateVisitor :public IStateVisitor<T> {
public:
    void visit(T& object) override {
        std::cout << __FUNCSIG__ << ":" << object << "\n";
    }
};


template<typename T>
void print(const T& obj) {
    State<T> state{ obj };
}

int main() {
    


    return 0;
}
