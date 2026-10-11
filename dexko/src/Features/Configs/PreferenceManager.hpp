#pragma once
#include <nlohmann/json.hpp>

struct Preferences
{
    std::string mDefaultConfigName;
    bool mFallbackToD3D11 = false;
    bool mEnforceDebugging = false;
};

class PreferenceManager {
public:
    static std::shared_ptr<Preferences> load();
    static void save(const std::shared_ptr<Preferences>& prefs);
};