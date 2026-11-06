#pragma once

#include <Features/Modules/Module.hpp>
#include <Features/FeatureManager.hpp>
#include <Features/Events/BaseTickEvent.hpp>
#include <Features/Events/PacketInEvent.hpp>
#include <Features/Events/BlockChangedEvent.hpp>
#include <Features/Events/RenderEvent.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Chunk/LevelChunk.hpp>
#include <SDK/Minecraft/World/Chunk/SubChunkBlockStorage.hpp>
#include <SDK/Minecraft/Network/Packets/UpdateBlockPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <unordered_map>
#include <mutex>

class SmartXRay : public ModuleBase<SmartXRay>
{
public:
    // ========== Ore State ==========
    enum class OreState : uint8_t {
        Unknown,
        Verifying,  // ждём результат проверки (polling)
        Real,
        Fake
    };

    struct OreEntry {
        glm::ivec3 pos;
        int blockId;
        OreState state = OreState::Unknown;
        ImColor color;
    };

    // Запись очереди верификации — после копки проверяем через N тиков
    struct VerifyEntry {
        BlockPos pos;
        int ticksLeft;
    };

    // ========== Settings ==========
    enum class DisplayMode { RealOnly, RealAndUnknown, All };
    enum class RenderStyle { Filled, Outline, Both };

    EnumSettingT<DisplayMode> mDisplayMode = EnumSettingT("Display", "What to show",
        DisplayMode::RealOnly, "Real Only", "Real + Unknown", "All");
    EnumSettingT<RenderStyle> mRenderStyle = EnumSettingT("Render", "Render style",
        RenderStyle::Outline, "Filled", "Outline", "Both");
    NumberSetting mRadius = NumberSetting("Radius", "Max distance", 40.f, 8.f, 128.f, 1.f);
    NumberSetting mChunkRadius = NumberSetting("Chunk Radius", "Scan radius", 4.f, 1.f, 16.f, 1.f);
    NumberSetting mVerifyTicks = NumberSetting("Verify Ticks", "Ticks to wait before checking (more = safer)", 5.f, 1.f, 20.f, 1.f);
    BoolSetting mShowStats = BoolSetting("Show Stats", "Show counter", true);
    BoolSetting mTracers = BoolSetting("Tracers", "Lines to real ores", false);
    BoolSetting mExposureCheck = BoolSetting("Exposure Check", "Instantly mark exposed ores as real", true);
    BoolSetting mDebugLog = BoolSetting("Debug Log", "Log real/fake detections to console", false);

    BoolSetting mDiamond = BoolSetting("Diamond", "", true);
    BoolSetting mEmerald = BoolSetting("Emerald", "", true);
    BoolSetting mGold = BoolSetting("Gold", "", true);
    BoolSetting mIron = BoolSetting("Iron", "", false);
    BoolSetting mCoal = BoolSetting("Coal", "", false);
    BoolSetting mRedstone = BoolSetting("Redstone", "", false);
    BoolSetting mLapis = BoolSetting("Lapis", "", true);
    BoolSetting mAncientDebris = BoolSetting("Ancient Debris", "", true);

    SmartXRay() : ModuleBase("SmartXRay",
        "X-Ray that filters fake ores from Anti X-Ray",
        ModuleCategory::Visual, 0, false)
    {
        addSettings(
            &mDisplayMode, &mRenderStyle, &mRadius, &mChunkRadius,
            &mVerifyTicks, &mShowStats, &mTracers, &mExposureCheck, &mDebugLog,
            &mDiamond, &mEmerald, &mGold, &mIron, &mCoal, &mRedstone, &mLapis, &mAncientDebris
        );
        mNames = {
            {Lowercase, "smartxray"}, {LowercaseSpaced, "smart xray"},
            {Normal, "SmartXRay"}, {NormalSpaced, "Smart X-Ray"}
        };
    }

    // ========== Block ID constants ==========
    static constexpr int C_DIAMOND_ORE = 56, C_DEEPSLATE_DIAMOND_ORE = 660;
    static constexpr int C_EMERALD_ORE = 129, C_DEEPSLATE_EMERALD_ORE = 662;
    static constexpr int C_GOLD_ORE = 14, C_DEEPSLATE_GOLD_ORE = 657;
    static constexpr int C_IRON_ORE = 15, C_DEEPSLATE_IRON_ORE = 656;
    static constexpr int C_COAL_ORE = 16, C_DEEPSLATE_COAL_ORE = 661;
    static constexpr int C_REDSTONE_ORE = 73, C_REDSTONE_ORE_LIT = 74;
    static constexpr int C_DEEPSLATE_REDSTONE_ORE = 658, C_DEEPSLATE_LIT_REDSTONE_ORE = 659;
    static constexpr int C_LAPIS_ORE = 21, C_DEEPSLATE_LAPIS_ORE = 655;
    static constexpr int C_ANCIENT_DEBRIS = 526, C_NETHER_GOLD_ORE = 543;

    // ========== State ==========
    std::mutex mMutex;
    std::unordered_map<BlockPos, OreEntry> mOres;
    std::vector<VerifyEntry> mVerifyQueue;
    int mRealCount = 0, mFakeCount = 0;

    // Spiral scan state
    ChunkPos mSearchCenter, mCurrentChunkPos;
    int mSubChunkIndex = 0, mDirectionIndex = 0, mSteps = 1, mStepsCount = 0;

    // ========== Helpers ==========

    bool isOreBlockId(int id) {
        switch (id) {
            case C_DIAMOND_ORE: case C_DEEPSLATE_DIAMOND_ORE:
            case C_EMERALD_ORE: case C_DEEPSLATE_EMERALD_ORE:
            case C_GOLD_ORE: case C_DEEPSLATE_GOLD_ORE:
            case C_IRON_ORE: case C_DEEPSLATE_IRON_ORE:
            case C_COAL_ORE: case C_DEEPSLATE_COAL_ORE:
            case C_REDSTONE_ORE: case C_REDSTONE_ORE_LIT:
            case C_DEEPSLATE_REDSTONE_ORE: case C_DEEPSLATE_LIT_REDSTONE_ORE:
            case C_LAPIS_ORE: case C_DEEPSLATE_LAPIS_ORE:
            case C_ANCIENT_DEBRIS: case C_NETHER_GOLD_ORE:
                return true;
            default: return false;
        }
    }

    bool isOreEnabled(int id) {
        switch (id) {
            case C_DIAMOND_ORE: case C_DEEPSLATE_DIAMOND_ORE: return mDiamond.mValue;
            case C_EMERALD_ORE: case C_DEEPSLATE_EMERALD_ORE: return mEmerald.mValue;
            case C_GOLD_ORE: case C_DEEPSLATE_GOLD_ORE: case C_NETHER_GOLD_ORE: return mGold.mValue;
            case C_IRON_ORE: case C_DEEPSLATE_IRON_ORE: return mIron.mValue;
            case C_COAL_ORE: case C_DEEPSLATE_COAL_ORE: return mCoal.mValue;
            case C_REDSTONE_ORE: case C_REDSTONE_ORE_LIT:
            case C_DEEPSLATE_REDSTONE_ORE: case C_DEEPSLATE_LIT_REDSTONE_ORE: return mRedstone.mValue;
            case C_LAPIS_ORE: case C_DEEPSLATE_LAPIS_ORE: return mLapis.mValue;
            case C_ANCIENT_DEBRIS: return mAncientDebris.mValue;
            default: return false;
        }
    }

    bool isAirOrLiquid(int id) {
        return id == 0 || (id >= 8 && id <= 11);
    }

    ImColor getOreColor(int id) {
        switch (id) {
            case C_DIAMOND_ORE: case C_DEEPSLATE_DIAMOND_ORE: return ImColor(0.f,1.f,1.f,1.f);
            case C_EMERALD_ORE: case C_DEEPSLATE_EMERALD_ORE: return ImColor(0.f,0.8f,0.f,1.f);
            case C_GOLD_ORE: case C_DEEPSLATE_GOLD_ORE: case C_NETHER_GOLD_ORE: return ImColor(1.f,0.84f,0.f,1.f);
            case C_IRON_ORE: case C_DEEPSLATE_IRON_ORE: return ImColor(0.82f,0.71f,0.55f,1.f);
            case C_COAL_ORE: case C_DEEPSLATE_COAL_ORE: return ImColor(0.2f,0.2f,0.2f,1.f);
            case C_REDSTONE_ORE: case C_REDSTONE_ORE_LIT:
            case C_DEEPSLATE_REDSTONE_ORE: case C_DEEPSLATE_LIT_REDSTONE_ORE: return ImColor(0.8f,0.f,0.f,1.f);
            case C_LAPIS_ORE: case C_DEEPSLATE_LAPIS_ORE: return ImColor(0.f,0.f,0.8f,1.f);
            case C_ANCIENT_DEBRIS: return ImColor(0.4f,0.2f,0.1f,1.f);
            default: return ImColor(1.f,1.f,1.f,1.f);
        }
    }

    // ========== КЛЮЧЕВОЕ: Проверка "exposed" (открытая руда = 100% реальная) ==========
    // Anti X-Ray фейкает ТОЛЬКО полностью закрытые блоки.
    // Если хотя бы 1 сторона руды — воздух/жидкость, она реальная.
    bool isExposed(const BlockPos& pos) {
        auto* bs = ClientInstance::get()->getBlockSource();
        if (!bs) return false;

        static const glm::ivec3 dirs[] = {
            {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
        };

        for (auto& d : dirs) {
            glm::ivec3 np = {pos.x + d.x, pos.y + d.y, pos.z + d.z};
            Block* neighbor = bs->getBlock(np);
            if (!neighbor || !neighbor->mLegacy) continue;
            int nid = neighbor->mLegacy->getBlockId();
            if (isAirOrLiquid(nid)) return true;
        }
        return false;
    }

    // ========== Добавить в очередь верификации ==========
    void queueVerify(const BlockPos& pos) {
        // Не дублировать
        for (auto& v : mVerifyQueue) {
            if (v.pos.x == pos.x && v.pos.y == pos.y && v.pos.z == pos.z) return;
        }
        mVerifyQueue.push_back({pos, (int)mVerifyTicks.mValue});
    }

    // ========== Reset ==========
    void reset() {
        std::lock_guard lock(mMutex);
        mOres.clear();
        mVerifyQueue.clear();
        mRealCount = 0; mFakeCount = 0;
        mStepsCount = 0; mSteps = 1; mDirectionIndex = 0; mSubChunkIndex = 0;
        auto* player = ClientInstance::get()->getLocalPlayer();
        if (player) {
            mSearchCenter = ChunkPos(*player->getPos());
            mCurrentChunkPos = mSearchCenter;
        }
    }

    // ========== Spiral scan ==========
    void moveToNext() {
        auto* bs = ClientInstance::get()->getBlockSource();
        if (!bs) return;
        static const std::vector<std::pair<int,int>> dirs = {{1,0},{0,1},{-1,0},{0,-1}};
        size_t numSub = (bs->getBuildHeight() - bs->getBuildDepth()) / 16;
        if (numSub - 1 > mSubChunkIndex) { mSubChunkIndex++; return; }
        mCurrentChunkPos.x += dirs[mDirectionIndex].first;
        mCurrentChunkPos.y += dirs[mDirectionIndex].second;
        mStepsCount++;
        if (mStepsCount >= mSteps) {
            mStepsCount = 0;
            mDirectionIndex = (mDirectionIndex + 1) % dirs.size();
            if (mDirectionIndex % 2 == 0) mSteps++;
        }
        mSubChunkIndex = 0;
    }

    void scanSubChunk(ChunkPos cp, int si) {
        auto* ci = ClientInstance::get();
        auto* player = ci->getLocalPlayer(); if (!player) return;
        auto* bs = ci->getBlockSource(); if (!bs) return;
        size_t numSub = (bs->getBuildHeight() - bs->getBuildDepth()) / 16;
        if (si < 0 || si >= (int)numSub) return;
        LevelChunk* chunk = bs->getChunk(cp); if (!chunk) return;
        auto subChunk = (*chunk->getSubChunks())[si];
        SubChunkBlockStorage* br = subChunk.blockReadPtr; if (!br) return;
        int subH = (bs->getBuildHeight() - bs->getBuildDepth()) / chunk->getSubChunks()->size();

        for (uint16_t x = 0; x < 16; x++) {
            for (uint16_t z = 0; z < 16; z++) {
                for (uint16_t y = 0; y < subH; y++) {
                    uint16_t eid = (x * 0x10 + z) * 0x10 + (y & 0xf);
                    const Block* found = br->getElement(eid);
                    if (!found || !found->mLegacy) continue;
                    int bid = found->mLegacy->getBlockId();

                    BlockPos pos;
                    pos.x = (cp.x * 16) + x;
                    pos.z = (cp.y * 16) + z;
                    pos.y = y + (subChunk.subchunkIndex * 16);

                    if (bid == 0) {
                        mOres.erase(pos);
                        continue;
                    }

                    if (!isOreBlockId(bid)) {
                        // Если раньше тут была руда, а теперь нет → фейк
                        auto it = mOres.find(pos);
                        if (it != mOres.end() && it->second.state != OreState::Fake) {
                            it->second.state = OreState::Fake;
                            mFakeCount++;
                            if (mDebugLog.mValue) {
                                spdlog::info("[SmartXRay] FAKE (scan): ({},{},{}) was ore, now blockId={}",
                                    pos.x, pos.y, pos.z, bid);
                            }
                        }
                        continue;
                    }

                    if (!mOres.contains(pos)) {
                        OreEntry entry;
                        entry.pos = pos;
                        entry.blockId = bid;
                        entry.color = getOreColor(bid);

                        // Exposure check — открытая руда = 100% реальная
                        if (mExposureCheck.mValue && isExposed(pos)) {
                            entry.state = OreState::Real;
                            mRealCount++;
                            if (mDebugLog.mValue) {
                                spdlog::info("[SmartXRay] REAL (exposed): ({},{},{}) blockId={}",
                                    pos.x, pos.y, pos.z, bid);
                            }
                        } else {
                            entry.state = OreState::Unknown;
                        }

                        mOres[pos] = entry;
                    }
                }
            }
        }
    }

    // ========== Lifecycle ==========
    void onEnable() override {
        gFeatureManager->mDispatcher->listen<BaseTickEvent, &SmartXRay::onBaseTickEvent>(this);
        gFeatureManager->mDispatcher->listen<BlockChangedEvent, &SmartXRay::onBlockChangedEvent>(this);
        gFeatureManager->mDispatcher->listen<PacketInEvent, &SmartXRay::onPacketInEvent>(this);
        gFeatureManager->mDispatcher->listen<RenderEvent, &SmartXRay::onRenderEvent, nes::event_priority::VERY_FIRST>(this);
        reset();
    }

    void onDisable() override {
        gFeatureManager->mDispatcher->deafen<BaseTickEvent, &SmartXRay::onBaseTickEvent>(this);
        gFeatureManager->mDispatcher->deafen<BlockChangedEvent, &SmartXRay::onBlockChangedEvent>(this);
        gFeatureManager->mDispatcher->deafen<PacketInEvent, &SmartXRay::onPacketInEvent>(this);
        gFeatureManager->mDispatcher->deafen<RenderEvent, &SmartXRay::onRenderEvent>(this);
        reset();
    }

    // ========== ГЛАВНЫЙ ТИК ==========
    void onBaseTickEvent(BaseTickEvent& event) {
        if (!ClientInstance::get()->getLevelRenderer()) { reset(); return; }
        std::lock_guard lock(mMutex);

        auto* player = ClientInstance::get()->getLocalPlayer(); if (!player) return;
        auto* bs = ClientInstance::get()->getBlockSource(); if (!bs) return;

        // ====== 1. ОБРАБОТКА ОЧЕРЕДИ ВЕРИФИКАЦИИ ======
        // Это КЛЮЧЕВОЕ: читаем BlockSource напрямую после задержки
        for (auto it = mVerifyQueue.begin(); it != mVerifyQueue.end();) {
            it->ticksLeft--;
            if (it->ticksLeft <= 0) {
                // Читаем что РЕАЛЬНО на этой позиции
                Block* block = bs->getBlock(it->pos);
                if (block && block->mLegacy) {
                    int currentId = block->mLegacy->getBlockId();
                    auto oreIt = mOres.find(it->pos);

                    if (oreIt != mOres.end() &&
                        oreIt->second.state != OreState::Real &&
                        oreIt->second.state != OreState::Fake)
                    {
                        if (isOreBlockId(currentId)) {
                            // Руда всё ещё на месте → РЕАЛЬНАЯ
                            oreIt->second.state = OreState::Real;
                            mRealCount++;
                            if (mDebugLog.mValue) {
                                spdlog::info("[SmartXRay] REAL (verified): ({},{},{}) still blockId={}",
                                    it->pos.x, it->pos.y, it->pos.z, currentId);
                            }
                        } else {
                            // Руда исчезла → ФЕЙК (сервер заменил на камень)
                            oreIt->second.state = OreState::Fake;
                            mFakeCount++;
                            if (mDebugLog.mValue) {
                                spdlog::info("[SmartXRay] FAKE (verified): ({},{},{}) was ore, now blockId={}",
                                    it->pos.x, it->pos.y, it->pos.z, currentId);
                            }
                        }
                    }
                }
                it = mVerifyQueue.erase(it);
            } else {
                ++it;
            }
        }

        // ====== 2. СПИРАЛЬНОЕ СКАНИРОВАНИЕ ======
        if (glm::distance(glm::vec2(mCurrentChunkPos), glm::vec2(mSearchCenter)) > mChunkRadius.mValue) {
            mSearchCenter = ChunkPos(*player->getPos());
            mCurrentChunkPos = mSearchCenter;
            mStepsCount = 0; mSteps = 1; mDirectionIndex = 0; mSubChunkIndex = 0;
        }

        for (int i = 0; i < 5; i++) {
            TRY_CALL([&]() { scanSubChunk(mCurrentChunkPos, mSubChunkIndex); });
            moveToNext();
        }

        // ====== 3. ОЧИСТКА ДАЛЁКИХ РУД ======
        glm::vec3 pp = *player->getPos();
        float maxD = mRadius.mValue * mRadius.mValue * 4.f;
        for (auto it = mOres.begin(); it != mOres.end();) {
            glm::vec3 op(it->first.x, it->first.y, it->first.z);
            if (glm::dot(op - pp, op - pp) > maxD) it = mOres.erase(it);
            else ++it;
        }
    }

    // ========== БЛОК СЛОМАН → ПРОВЕРЯЕМ СОСЕДЕЙ ==========
    void onBlockChangedEvent(BlockChangedEvent& event) {
        if (!ClientInstance::get()->getLevelRenderer()) return;
        std::lock_guard lock(mMutex);

        int newId = event.mNewBlock->mLegacy->getBlockId();
        int oldId = event.mOldBlock->mLegacy->getBlockId();
        BlockPos pos = event.mBlockPos;

        if (mDebugLog.mValue) {
            spdlog::info("[SmartXRay] BlockChanged: ({},{},{}) {} -> {}",
                pos.x, pos.y, pos.z, oldId, newId);
        }

        // Блок стал воздухом (кто-то сломал)
        // → Ставим соседние руды на верификацию
        if (isAirOrLiquid(newId) && !isAirOrLiquid(oldId)) {
            static const glm::ivec3 dirs[] = {
                {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
            };

            for (auto& d : dirs) {
                BlockPos np = {pos.x + d.x, pos.y + d.y, pos.z + d.z};
                auto it = mOres.find(np);
                if (it != mOres.end() && it->second.state == OreState::Unknown) {
                    it->second.state = OreState::Verifying;
                    queueVerify(np);

                    if (mDebugLog.mValue) {
                        spdlog::info("[SmartXRay] Queued verify: ({},{},{})",
                            np.x, np.y, np.z);
                    }
                }
            }
        }

        // Руда исчезла (стала камнем/воздухом)
        if (isOreBlockId(oldId) && !isOreBlockId(newId)) {
            auto it = mOres.find(pos);
            if (it != mOres.end() && it->second.state != OreState::Fake) {
                it->second.state = OreState::Fake;
                mFakeCount++;
                if (mDebugLog.mValue) {
                    spdlog::info("[SmartXRay] FAKE (event): ({},{},{}) ore disappeared",
                        pos.x, pos.y, pos.z);
                }
            }
        }

        // Новая руда появилась
        if (isOreBlockId(newId) && !mOres.contains(pos)) {
            OreEntry e;
            e.pos = pos; e.blockId = newId; e.color = getOreColor(newId);
            e.state = (mExposureCheck.mValue && isExposed(pos)) ? OreState::Real : OreState::Unknown;
            if (e.state == OreState::Real) mRealCount++;
            mOres[pos] = e;
        }
    }

    // ========== ПАКЕТЫ ==========
    void onPacketInEvent(PacketInEvent& event) {
        if (!ClientInstance::get()->getLevelRenderer()) { reset(); return; }
        if (event.mPacket->getId() == PacketID::ChangeDimension) { reset(); return; }
        if (event.mPacket->getId() == PacketID::PlayerAction) {
            auto p = event.getPacket<PlayerActionPacket>();
            if (p->mAction == PlayerActionType::Respawn) reset();
        }

        // UpdateBlock — ещё один источник данных
        if (event.mPacket->getId() == PacketID::UpdateBlock) {
            auto pkt = event.getPacket<UpdateBlockPacket>();
            if (pkt->mLayer != UpdateBlockPacket::BlockLayer::Standard) return;

            std::lock_guard lock(mMutex);
            BlockPos pos = {pkt->mPos.x, pkt->mPos.y, pkt->mPos.z};

            // Если на этой позиции есть руда в нашей карте
            // и она ещё не проверена — ставим на верификацию
            auto it = mOres.find(pos);
            if (it != mOres.end() && it->second.state == OreState::Unknown) {
                it->second.state = OreState::Verifying;
                queueVerify(pos);
            }

            // Также проверяем соседей
            static const glm::ivec3 dirs[] = {
                {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
            };
            for (auto& d : dirs) {
                BlockPos np = {pos.x + d.x, pos.y + d.y, pos.z + d.z};
                auto nit = mOres.find(np);
                if (nit != mOres.end() && nit->second.state == OreState::Unknown) {
                    nit->second.state = OreState::Verifying;
                    queueVerify(np);
                }
            }
        }
    }

    // ========== РЕНДЕР ==========
    void onRenderEvent(RenderEvent& event) {
        if (!ClientInstance::get()->getLevelRenderer()) { reset(); return; }
        if (ClientInstance::get()->getMouseGrabbed()) return;
        std::lock_guard lock(mMutex);

        auto* player = ClientInstance::get()->getLocalPlayer(); if (!player) return;
        auto dl = ImGui::GetBackgroundDrawList();
        glm::vec3 pp = *player->getPos();
        float rSq = mRadius.mValue * mRadius.mValue;

        int dispReal = 0, dispVerify = 0, dispUnk = 0;

        for (auto& [pos, e] : mOres) {
            if (!isOreEnabled(e.blockId)) continue;

            // Фильтр по режиму отображения
            switch (mDisplayMode.mValue) {
                case DisplayMode::RealOnly:
                    if (e.state != OreState::Real && e.state != OreState::Verifying) continue;
                    break;
                case DisplayMode::RealAndUnknown:
                    if (e.state == OreState::Fake) continue;
                    break;
                default: break;
            }

            // Фильтр по расстоянию
            glm::vec3 op(pos.x, pos.y, pos.z);
            if (glm::dot(op - pp, op - pp) > rSq) continue;

            AABB aabb(pos, glm::vec3(1.f));
            auto pts = MathUtils::getImBoxPoints(aabb);
            if (pts.empty()) continue;

            ImColor c = e.color;

            if (e.state == OreState::Real) {
                dispReal++;
                // Яркий + зелёная обводка
                if (mRenderStyle.mValue != RenderStyle::Filled) {
                    dl->AddPolyline(pts.data(), pts.size(), c, 0, 2.5f);
                    dl->AddPolyline(pts.data(), pts.size(), ImColor(0.f,1.f,0.f,0.4f), 0, 4.f);
                }
                if (mRenderStyle.mValue != RenderStyle::Outline) {
                    dl->AddConvexPolyFilled(pts.data(), pts.size(),
                        ImColor(c.Value.x, c.Value.y, c.Value.z, 0.35f));
                }

                // Трейсер
                if (mTracers.mValue) {
                    ImVec2 sc = {ImGui::GetIO().DisplaySize.x/2.f, ImGui::GetIO().DisplaySize.y/2.f};
                    ImVec2 bc = {0,0};
                    for (auto& p : pts) { bc.x += p.x; bc.y += p.y; }
                    bc.x /= pts.size(); bc.y /= pts.size();
                    dl->AddLine(sc, bc, ImColor(c.Value.x,c.Value.y,c.Value.z,0.5f), 1.5f);
                }
            }
            else if (e.state == OreState::Verifying) {
                dispVerify++;
                float pulse = (sinf((float)NOW * 0.005f) + 1.f) / 2.f;
                float a = 0.3f + pulse * 0.4f;
                if (mRenderStyle.mValue != RenderStyle::Filled)
                    dl->AddPolyline(pts.data(), pts.size(), ImColor(1.f,1.f,0.f,a), 0, 2.f);
                if (mRenderStyle.mValue != RenderStyle::Outline)
                    dl->AddConvexPolyFilled(pts.data(), pts.size(), ImColor(1.f,1.f,0.f,a*0.3f));
            }
            else if (e.state == OreState::Unknown) {
                dispUnk++;
                if (mRenderStyle.mValue != RenderStyle::Filled)
                    dl->AddPolyline(pts.data(), pts.size(), ImColor(c.Value.x,c.Value.y,c.Value.z,0.2f), 0, 1.f);
                if (mRenderStyle.mValue != RenderStyle::Outline)
                    dl->AddConvexPolyFilled(pts.data(), pts.size(), ImColor(c.Value.x,c.Value.y,c.Value.z,0.08f));
            }
            else if (e.state == OreState::Fake && mDisplayMode.mValue == DisplayMode::All) {
                dl->AddPolyline(pts.data(), pts.size(), ImColor(1.f,0.f,0.f,0.15f), 0, 1.f);
            }
        }

        // Статистика
        if (mShowStats.mValue) {
            float x = 10.f, y = ImGui::GetIO().DisplaySize.y - 110.f;
            char buf[128];

            snprintf(buf, sizeof(buf), "Smart X-Ray v2");
            dl->AddText({x,y}, IM_COL32(255,255,255,255), buf); y += 16.f;

            snprintf(buf, sizeof(buf), "Real: %d  Fake: %d", mRealCount, mFakeCount);
            dl->AddText({x,y}, IM_COL32(0,255,0,255), buf); y += 14.f;

            snprintf(buf, sizeof(buf), "Verifying: %d  Unknown: %d",
                dispVerify, dispUnk);
            dl->AddText({x,y}, IM_COL32(255,255,0,200), buf); y += 14.f;

            snprintf(buf, sizeof(buf), "Queue: %d  Total: %d",
                (int)mVerifyQueue.size(), (int)mOres.size());
            dl->AddText({x,y}, IM_COL32(200,200,200,200), buf); y += 14.f;

            int total = mRealCount + mFakeCount;
            if (total > 0) {
                float fp = (float)mFakeCount / total * 100.f;
                snprintf(buf, sizeof(buf), "Fake rate: %.1f%% (%d/%d)", fp, mFakeCount, total);
                dl->AddText({x,y}, fp > 80.f ? IM_COL32(255,50,50,255) : IM_COL32(255,200,100,255), buf);
            }
        }
    }
};