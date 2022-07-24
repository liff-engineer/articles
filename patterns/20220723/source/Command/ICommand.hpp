#pragma once
#include <functional>

class UiContext;
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void start(UiContext* ctx, std::function<void(ICommand*)> stopNotifyer) = 0;
    virtual void stop() = 0;
};
