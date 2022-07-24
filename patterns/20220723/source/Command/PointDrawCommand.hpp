#pragma once

#include "ICommand.hpp"
#include "UiService/UiService.hpp"

class PointDrawCommand :public  ICommand {
public:
    void start(UiContext* ctx, std::function<void(ICommand*)> stopNotifyer) override;
    void stop() override;
private:
    std::function<void(ICommand*)> m_stopNotifyer;
    std::unique_ptr<UiService> m_input;
};
