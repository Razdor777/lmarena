#pragma once
//
// Detour — обёртка над MinHook.
//
// Кроме самого перехвата здесь есть журнал: какие перехваты вообще создались и
// какие включились. Раньше это было видно только в логе, а лог в игре не
// посмотришь — поэтому «модуль не работает» приходилось выяснять наугад.
// Теперь состояние хуков можно спросить из чата: .diag
//

#include <string>
#include <vector>

class Detour {
public:
    // Запись журнала. addr == nullptr и created == false — хук не создался.
    struct Status
    {
        std::string name;
        void*       addr    = nullptr;
        bool        created = false;
        bool        enabled = false;
    };

    // Все перехваты, которые пытались создать, по порядку.
    static std::vector<Status> snapshot();

    void* mFunc{};
    void* mOriginalFunc{};

    std::string mName;

    ~Detour();
    Detour(const std::string& name, void* addr, void* detour, bool silent = false);

    void enable(bool silent = false) const;
    void restore() const; // provided just in case, you should NOT use this

    template <auto T>
    auto getOriginal() {
        return reinterpret_cast<decltype(T)>(mOriginalFunc);
    }

private:
    // Индекс своей записи в журнале. mutable, потому что enable() const.
    mutable int mSlot = -1;
};
