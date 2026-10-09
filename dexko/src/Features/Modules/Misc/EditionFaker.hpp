#pragma once
//
// EditionFaker.hpp
//
// Module that spoofs the client's OS/edition field in the connection request,
// making the server (and anti-cheats) believe the player is on a different
// platform (Android, iOS, Windows, etc.).
//
// DeviceOS indices used by MCBE:
//   0 = Unknown
//   1 = Android
//   2 = iOS
//   3 = OSX / macOS
//   4 = FireOS / Amazon
//   5 = GearVR
//   6 = Hololens
//   7 = Windows 10 (UWP)
//   8 = Win32
//   9 = Dedicated
//  10 = tvOS
//  11 = PlayStation 4
//  12 = Nintendo Switch
//  13 = Xbox One
//  14 = Windows Phone
//

#include <Features/Modules/Module.hpp>
#include <Features/Modules/Setting.hpp>

class EditionFaker : public ModuleBase<EditionFaker>
{
public:
    // OS selection setting.  Default = 7 (Windows 10 UWP) so behaviour is
    // identical to the old hard-coded value DeviceSpoof used before this
    // module was introduced.
    EnumSetting mOs = EnumSetting(
        "OS", "The operating system to spoof",
        6,   // index 6 = "Windows" in the list below
        std::vector<std::string>{
            "Unknown",   // 0
            "Android",   // 1
            "iOS",       // 2
            "macOS",     // 3
            "FireOS",    // 4
            "GearVR",    // 5
            "Windows",   // 6  → DeviceOS 7
            "Win32",     // 7  → DeviceOS 8
        }
    );

    EditionFaker() : ModuleBase("EditionFaker", "Spoofs your platform edition", ModuleCategory::Misc, 0, false)
    {
        mNames = {
            {Lowercase,       "editionfaker"},
            {LowercaseSpaced, "edition faker"},
            {Normal,          "EditionFaker"},
            {NormalSpaced,    "Edition Faker"},
        };
        addSetting(&mOs);
    }
};
