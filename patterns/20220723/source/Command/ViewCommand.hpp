#pragma once

#include "ICommand.hpp"
#include <memory>
#include <QObject>

class ViewCommand :public  ICommand {
public:
    void start(UiContext* ctx, std::function<void(ICommand*)> stopNotifyer) override;
    void stop() override;
private:
    std::function<void(ICommand*)> m_stopNotifyer;
    std::unique_ptr<QObject> m_inputer;
};
