#include "UiService/UiService.hpp"

void UiService::stop()
{
    m_channels.visit<IInputer>([](IInputer* inputer) { inputer->stop(); });
}
