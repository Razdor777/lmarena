#pragma once

class AudioUtils {
public:
    static std::vector<std::string> listMicrophones();
    static void recordVoiceClip(std::function<void(const std::vector<BYTE>&)> onAudioDataReady);
};