#pragma once
#include <string>
#include <Utils/Resource.hpp>
#include <Windows.h>

class SoundUtils {
public:
    static void playSoundFromEmbeddedResource(std::string resourceName, float volume);
};
