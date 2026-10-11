#pragma once
//
//


class PreGameHook : public Hook {
public:
    PreGameHook() : Hook() {
        mName = "ClientInstance::isPreGame";
    }

    static std::unique_ptr<Detour> mDetour;

    static bool onPreGame(void* _this);
    void init() override;
};