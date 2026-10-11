#pragma once
#include <string>
#include <spdlog/fmt/fmt.h>   // fmt::format_string / fmt::format come from spdlog's bundled fmt
//

class ChatUtils {
public:
    // Chat input bridge used by KeyHook: while the player is typing in the
    // client-side chat, KeyHook refuses to forward input to the game (the
    // character stands still, the camera does not turn), so the keys reach the
    // text field instead. sChatSwallowKey is the key whose release has to be
    // eaten right after the chat closes — the vanilla screen opens on Enter's
    // release, and letting it through would open the original chat on top.
    static inline bool  sChatInputActive      = false;
    static inline int   sChatSwallowKey       = 0;
    static inline float sChatSwallowDeadline  = 0.f;

    // Optional sink: when set, queued client messages go here instead of the
    // vanilla chat (used by BaseTickHook when a client-side chat is active).
    static inline void (*sSink)(const std::string&) = nullptr;

    static void displayClientMessage(const std::string& msg);
    template<typename... Args>
    static void displayClientMessage(fmt::format_string<Args...> fmt, Args&&... args)
    {
        displayClientMessage(fmt::format(fmt, std::forward<Args>(args)...));
    }
};
