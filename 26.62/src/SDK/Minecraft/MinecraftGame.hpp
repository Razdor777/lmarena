#pragma once
#include <string>
//
//


class MinecraftGame {
public:
    static MinecraftGame* getInstance();
    class ClientInstance* getPrimaryClientInstance();
    class UIProfanityContext* getProfanityContext();
    bool getMouseGrabbed();
    void setMouseGrabbed(bool grabbed);
    std::string getxuid();
    void playUi(const std::string& soundName, float volume, float pitch);
};