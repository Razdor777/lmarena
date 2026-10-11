//
//

#include "MainView.hpp"
#include <SDK/OffsetProvider.hpp>
#include <SDK/SigManager.hpp>

class BedrockPlatformUWP* MainView::getBedrockPlatform()
{
    return hat::member_at<BedrockPlatformUWP*>(this, OffsetProvider::MainView_bedrockPlatform);
}

MainView* MainView::getInstance()
{
    static auto mainView = reinterpret_cast<MainView**>(SigManager::MainView_instance);
    return *mainView;
}