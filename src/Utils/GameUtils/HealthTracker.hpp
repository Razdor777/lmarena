#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <regex>
#include <chrono>

class HealthTracker {
public:
    static HealthTracker& getInstance() {
        static HealthTracker instance;
        return instance;
    }

    struct HealthInfo {
        float health = 20.f;
        float maxHealth = 20.f;
        int64_t lastUpdate = 0;
        std::string rawText;
    };

    // Парсим HP из текста сервера
    // Формат твоего сервера: "13 ❤ НИК | комбо 1 удар"
    // Число в начале = HP
    bool parseHealthText(const std::string& text, const std::string& playerName) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto now = getNow();

        // Убираем цветовые коды §X
        std::string clean = removeColorCodes(text);

        float hp = -1.f;
        float maxHp = 20.f;
        bool found = false;

        // === Формат 1: "HP/MaxHP" (15.5/20) ===
        {
            std::regex r("(\\d+\\.?\\d*)\\s*/\\s*(\\d+\\.?\\d*)");
            std::smatch m;
            if (std::regex_search(clean, m, r)) {
                hp = std::stof(m[1].str());
                maxHp = std::stof(m[2].str());
                found = true;
            }
        }

        // === Формат 2: Число в начале строки (твой сервер) ===
        // "13 ❤ НИК | комбо 1 удар"
        if (!found) {
            // Убираем пробелы в начале
            std::string trimmed = clean;
            size_t start = trimmed.find_first_not_of(" \t");
            if (start != std::string::npos) trimmed = trimmed.substr(start);

            std::regex r("^(\\d+\\.?\\d*)");
            std::smatch m;
            if (std::regex_search(trimmed, m, r)) {
                float val = std::stof(m[1].str());
                if (val >= 0.f && val <= 40.f) {
                    hp = val;
                    found = true;
                }
            }
        }

        // === Формат 3: "HP: X" или "Health: X" ===
        if (!found) {
            std::regex r("(?:HP|Health|hp|health)\\s*[:=]\\s*(\\d+\\.?\\d*)");
            std::smatch m;
            if (std::regex_search(clean, m, r)) {
                hp = std::stof(m[1].str());
                found = true;
            }
        }

        // === Формат 4: Число рядом с сердечком ===
        if (!found) {
            // Число ДО сердечка
            std::regex r1("(\\d+\\.?\\d*)\\s*[^\\w]");
            std::smatch m;
            if (std::regex_search(clean, m, r1)) {
                float val = std::stof(m[1].str());
                if (val >= 0.f && val <= 40.f) {
                    hp = val;
                    found = true;
                }
            }
        }

        if (found && !playerName.empty()) {
            mHealthMap[playerName] = {hp, maxHp, now, text};
            return true;
        }
        return false;
    }

    // Получить HP игрока
    // staleMs — через сколько мс данные считаются устаревшими
    bool getHealth(const std::string& name, float& outHp, float& outMaxHp, int64_t staleMs = 10000) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mHealthMap.find(name);
        if (it == mHealthMap.end()) return false;

        auto now = getNow();
        // Данные полностью устарели (30 сек) — не показываем
        if (now - it->second.lastUpdate > 30000) return false;

        outHp = it->second.health;
        outMaxHp = it->second.maxHealth;
        return true;
    }

    // Проверяет устарели ли данные (больше staleMs мс)
    bool isStale(const std::string& name, int64_t staleMs = 5000) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mHealthMap.find(name);
        if (it == mHealthMap.end()) return true;
        return (getNow() - it->second.lastUpdate) > staleMs;
    }

    // Сколько секунд прошло с последнего обновления
    float getSecondsSinceUpdate(const std::string& name) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mHealthMap.find(name);
        if (it == mHealthMap.end()) return 999.f;
        return (float)(getNow() - it->second.lastUpdate) / 1000.f;
    }

    void setLastAttacked(const std::string& name) {
        std::lock_guard<std::mutex> lock(mMutex);
        mLastAttacked = name;
        mLastAttackTime = getNow();
    }

    std::string getLastAttacked() {
        std::lock_guard<std::mutex> lock(mMutex);
        if (getNow() - mLastAttackTime > 3000) return "";
        return mLastAttacked;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);
        mHealthMap.clear();
    }

private:
    HealthTracker() = default;

    std::string removeColorCodes(const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (size_t i = 0; i < text.size(); i++) {
            // §X (UTF-8: 0xC2 0xA7 + 1 байт кода)
            if (i + 2 < text.size() &&
                (unsigned char)text[i] == 0xC2 &&
                (unsigned char)text[i+1] == 0xA7) {
                i += 2; // пропускаем § и код цвета
                continue;
            }
            result += text[i];
        }
        return result;
    }

    int64_t getNow() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    std::mutex mMutex;
    std::unordered_map<std::string, HealthInfo> mHealthMap;
    std::string mLastAttacked;
    int64_t mLastAttackTime = 0;
};