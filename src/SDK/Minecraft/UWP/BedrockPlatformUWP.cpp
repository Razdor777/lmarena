//
//

#include "BedrockPlatformUWP.hpp"

#include <SDK/OffsetProvider.hpp>

MinecraftGame* BedrockPlatformUWP::getMinecraftGame()
{
    return nullptr;
    //return hat::member_at<MinecraftGame*>(this, OffsetProvider::BedrockPlatformUWP_mcGame);
}
