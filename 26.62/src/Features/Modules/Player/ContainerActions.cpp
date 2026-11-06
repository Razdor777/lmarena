#include "ContainerActions.hpp"
#include <Features/FeatureManager.hpp>
#include <Hook/Hooks/ContainerHooks/ContainerScreenControllerHook.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Inventory/ContainerManagerModel.hpp>
#include <SDK/Minecraft/Inventory/ContainerScreenController.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/Network/Packets/InventoryTransactionPacket.hpp>
#include <SDK/Minecraft/Network/Packets/PlayerActionPacket.hpp>
#include <Utils/GameUtils/ItemUtils.hpp>
#include <Utils/MiscUtils/NotifyUtils.hpp>

// ==================== LIFECYCLE ====================

void ContainerActions::onEnable()
{
    gFeatureManager->mDispatcher->listen<RenderEvent,
        &ContainerActions::onRenderEvent>(this);
    gFeatureManager->mDispatcher->listen<ContainerScreenTickEvent,
        &ContainerActions::onContainerScreenTickEvent>(this);
    gFeatureManager->mDispatcher->listen<BaseTickEvent,
        &ContainerActions::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent,
        &ContainerActions::onPacketInEvent>(this);
    resetAction();
}

void ContainerActions::onDisable()
{
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ContainerActions::onRenderEvent>(this);
    gFeatureManager->mDispatcher->deafen<ContainerScreenTickEvent, &ContainerActions::onContainerScreenTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &ContainerActions::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &ContainerActions::onPacketInEvent>(this);
    resetAction();
}

void ContainerActions::startAction(Action act) {
    if (mAction != Action::None) return;
    mAction = act;
    mActionSlot = 0;
    mActionPhase = 0;
    mLastActionTime = 0;
    mSortUseInPlace = false;
    mChestSize = 0;
    mSortOrder.clear();
    mSortOrderIdx = 0;
    mSortSwaps.clear();
    mSortSwapIdx = 0;
    mSortSubStep = 0;
}

void ContainerActions::resetAction() {
    mAction = Action::None;
    mActionSlot = 0;
    mActionPhase = 0;
    mLastActionTime = 0;
    mSortUseInPlace = false;
    mChestSize = 0;
    mSortOrder.clear();
    mSortOrderIdx = 0;
    mSortSwaps.clear();
    mSortSwapIdx = 0;
    mSortSubStep = 0;
}

// ==================== HELPERS ====================

int ContainerActions::countFreeInvSlots(Actor* player) {
    auto supplies = player->getSupplies();
    if (!supplies) return 0;
    auto container = supplies->getContainer();
    if (!container) return 0;
    int free = 0;
    for (int i = 0; i < 36; i++) {
        ItemStack* item = container->getItem(i);
        if (!item || !item->mItem) free++;
    }
    return free;
}

int ContainerActions::countChestItems(Actor* player) {
    auto cmm = player->getContainerManagerModel();
    if (!cmm) return 0;
    int count = 0;
    for (int i = 0; i < 54; i++) {
        TRY_CALL([&]() {
            ItemStack* item = cmm->getSlot(i);
            if (item && item->mItem) count++;
        });
    }
    return count;
}

// ==================== BUTTON ====================

bool ContainerActions::drawButton(ImDrawList* dl, ImVec2 pos, ImVec2 size,
                                   const char* label, bool disabled, bool active)
{
    ImVec2 mn = pos, mx = {pos.x + size.x, pos.y + size.y};
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool hovered = !disabled && mouse.x >= mn.x && mouse.x <= mx.x
                             && mouse.y >= mn.y && mouse.y <= mx.y;
    bool clicked = hovered && ImGui::GetIO().MouseClicked[0];
    ImColor bg = disabled  ? ImColor(0.15f,0.15f,0.15f,0.6f)
               : active   ? ImColor(0.1f,0.4f,0.8f,0.7f)
               : hovered  ? ImColor(0.25f,0.25f,0.25f,0.8f)
                           : ImColor(0.12f,0.12f,0.12f,0.75f);
    dl->AddRectFilled(mn, mx, bg, 6.f);
    ImColor border = active ? ImColor(0.3f,0.6f,1.f,0.9f)
                  : hovered ? ImColor(0.5f,0.5f,0.5f,0.8f)
                            : ImColor(0.3f,0.3f,0.3f,0.5f);
    dl->AddRect(mn, mx, border, 6.f, 0, 1.5f);
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText({pos.x+(size.x-ts.x)*0.5f, pos.y+(size.y-ts.y)*0.5f},
                disabled ? ImColor(0.4f,0.4f,0.4f,1.f) : ImColor(1.f,1.f,1.f,1.f), label);
    return clicked;
}

// ==================== RENDER ====================

void ContainerActions::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) {
        if (mAction != Action::None) resetAction();
        ContainerScreenControllerHook::sScreenActive = false;
        return;
    }

    // Дополнительная проверка через ContainerManagerModel
    auto* cmm = player->getContainerManagerModel();
    bool reallyOpen = cmm &&
        (cmm->mContainerType == ContainerType::Container ||
         cmm->mContainerType == ContainerType::Inventory);

    if (!reallyOpen) {
        ContainerScreenControllerHook::sScreenActive = false;
        ContainerScreenControllerHook::sActiveType   = ContainerType::None;
        if (mAction != Action::None) resetAction();
        return;
    }

    if (!ContainerScreenControllerHook::isScreenActive()) {
        if (mAction != Action::None) resetAction();
        return;
    }

    auto dl     = ImGui::GetForegroundDrawList();
    auto screen = ImGui::GetIO().DisplaySize;
    auto type   = ContainerScreenControllerHook::sActiveType;
    const float btnW = 120.f, btnH = 30.f, gap = 10.f;
    bool busy = (mAction != Action::None);

    if (type == ContainerType::Container) {
        float totalW = btnW*3 + gap*2;
        float x = (screen.x - totalW)*0.5f, y = screen.y*0.08f;

        {
            std::string lbl = mAction==Action::TakeAll
                ? "Taking ("+std::to_string(mActionSlot)+")" : "Take All";
            if (drawButton(dl, {x,y}, {btnW,btnH}, lbl.c_str(),
                           busy && mAction!=Action::TakeAll, mAction==Action::TakeAll))
                mAction==Action::TakeAll ? resetAction() : startAction(Action::TakeAll);
        }
        {
            std::string lbl = mAction==Action::StoreAll
                ? "Storing ("+std::to_string(mActionSlot)+")" : "Store All";
            if (drawButton(dl, {x+btnW+gap,y}, {btnW,btnH}, lbl.c_str(),
                           busy && mAction!=Action::StoreAll, mAction==Action::StoreAll))
                mAction==Action::StoreAll ? resetAction() : startAction(Action::StoreAll);
        }
        {
            std::string lbl;
            if (mAction == Action::SortChest) {
                if (mSortUseInPlace)
                    lbl = "Sort IP (" + std::to_string(mSortSwapIdx) + "/" + std::to_string(mSortSwaps.size()) + ")";
                else if (mActionPhase <= 1)
                    lbl = "Sort: Take (" + std::to_string(mActionSlot) + ")";
                else if (mActionPhase == 3)
                    lbl = "Sort: Put (" + std::to_string(mSortOrderIdx) + "/" + std::to_string(mSortOrder.size()) + ")";
                else
                    lbl = "Sorting...";
            } else {
                lbl = "Sort Chest";
            }
            if (drawButton(dl, {x+(btnW+gap)*2,y}, {btnW,btnH}, lbl.c_str(),
                           busy && mAction!=Action::SortChest, mAction==Action::SortChest))
                mAction==Action::SortChest ? resetAction() : startAction(Action::SortChest);
        }
    }
    else if (type == ContainerType::Inventory) {
        float x = (screen.x-btnW)*0.5f, y = screen.y*0.08f;
        std::string lbl = mAction==Action::DropAll
            ? "Dropping ("+std::to_string(mActionSlot)+"/40)" : "Drop All";
        if (drawButton(dl, {x,y}, {btnW,btnH}, lbl.c_str(),
                       busy && mAction!=Action::DropAll, mAction==Action::DropAll))
            mAction==Action::DropAll ? resetAction() : startAction(Action::DropAll);
    }
}

// ==================== EVENTS ====================

void ContainerActions::onBaseTickEvent(BaseTickEvent& event) {
    if (mAction != Action::DropAll) return;
    if (!ContainerScreenControllerHook::isScreenActive()) { resetAction(); return; }
    if (!event.mActor) { resetAction(); return; }
    processDropAll(event.mActor);
}

void ContainerActions::onPacketInEvent(PacketInEvent& event) {
    if (event.mPacket->getId() == PacketID::ChangeDimension) resetAction();
    if (event.mPacket->getId() == PacketID::PlayerAction) {
        auto pkt = event.getPacket<PlayerActionPacket>();
        if (pkt->mAction == PlayerActionType::Respawn) resetAction();
    }
}

void ContainerActions::onContainerScreenTickEvent(ContainerScreenTickEvent& event) {
    if (mAction!=Action::TakeAll && mAction!=Action::StoreAll && mAction!=Action::SortChest) return;
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) { resetAction(); return; }
    auto cmm = player->getContainerManagerModel();
    if (!cmm || cmm->mContainerType != ContainerType::Container) { resetAction(); return; }
    auto csc = event.mController;
    if (!csc) { resetAction(); return; }

    switch (mAction) {
        case Action::TakeAll:   processTakeAll(csc, player); break;
        case Action::StoreAll:  processStoreAll(csc, player); break;
        case Action::SortChest: processSortChest(csc, player); break;
        default: break;
    }
}

// ==================== DROP ALL ====================

void ContainerActions::processDropAll(Actor* player) {
    uint64_t delay = static_cast<uint64_t>(mActionDelay.mValue);
    if (delay > 0 && NOW - mLastActionTime < delay) return;
    auto supplies = player->getSupplies(); if (!supplies) { resetAction(); return; }
    auto container = supplies->getContainer(); if (!container) { resetAction(); return; }
    auto armorCont = player->getArmorContainer(); if (!armorCont) { resetAction(); return; }
    auto sender = ClientInstance::get()->getPacketSender(); if (!sender) { resetAction(); return; }
    static ItemStack blank;

    if (mActionPhase == 0) {
        int dropped = 0;
        while (mActionSlot < 36 && dropped < 5) {
            ItemStack* item = container->getItem(mActionSlot);
            if (item && item->mItem) { container->dropSlot(mActionSlot); dropped++; }
            mActionSlot++;
        }
        if (mActionSlot >= 36) { mActionPhase = 1; mActionSlot = 0; }
        mLastActionTime = NOW; return;
    }
    if (mActionPhase == 1) {
        while (mActionSlot < 4) {
            ItemStack* armor = armorCont->getItem(mActionSlot);
            if (armor && armor->mItem) {
                InventoryAction fromArmor(mActionSlot, armor, &blank);
                fromArmor.mSource.mType = InventorySourceType::ContainerInventory;
                fromArmor.mSource.mContainerId = static_cast<int>(ContainerID::Armor);
                InventoryAction toWorld(0, &blank, armor);
                toWorld.mSource.mType = InventorySourceType::WorldInteraction;
                toWorld.mSource.mContainerId = static_cast<int>(ContainerID::None);
                auto pkt = MinecraftPackets::createPacket<InventoryTransactionPacket>();
                auto cit = std::make_unique<ComplexInventoryTransaction>();
                cit->data.addAction(fromArmor); cit->data.addAction(toWorld);
                pkt->mTransaction = std::move(cit);
                sender->sendToServer(pkt.get());
                armor->mItem=nullptr; armor->mCount=0; armor->mAuxValue=0;
                armor->mBlock=nullptr; armor->mCompoundTag=nullptr; armor->valid=false;
            }
            mActionSlot++;
        }
        NotifyUtils::notify("Dropped all items!", 2.f, Notification::Type::Info);
        resetAction();
    }
}

// ==================== TAKE ALL ====================

void ContainerActions::processTakeAll(ContainerScreenController* csc, Actor* player) {
    uint64_t delay = static_cast<uint64_t>(mActionDelay.mValue);
    if (delay > 0 && NOW - mLastActionTime < delay) return;
    auto cmm = player->getContainerManagerModel(); if (!cmm) { resetAction(); return; }
    while (mActionSlot < 54) {
        bool found = false;
        TRY_CALL([&]() {
            ItemStack* item = cmm->getSlot(mActionSlot);
            if (item && item->mItem) {
                csc->handleAutoPlace("container_items", mActionSlot);
                mLastActionTime = NOW; found = true;
            }
        });
        mActionSlot++;
        if (found) return;
    }
    NotifyUtils::notify("Took all items!", 2.f, Notification::Type::Info);
    resetAction();
}

// ==================== STORE ALL ====================

void ContainerActions::processStoreAll(ContainerScreenController* csc, Actor* player) {
    uint64_t delay = static_cast<uint64_t>(mActionDelay.mValue);
    if (delay > 0 && NOW - mLastActionTime < delay) return;
    auto supplies = player->getSupplies(); if (!supplies) { resetAction(); return; }
    auto container = supplies->getContainer(); if (!container) { resetAction(); return; }

    if (mActionPhase == 0) {
        while (mActionSlot < 27) {
            ItemStack* item = container->getItem(mActionSlot + 9);
            if (item && item->mItem) {
                csc->handleAutoPlace("inventory_items", mActionSlot);
                mLastActionTime = NOW; mActionSlot++; return;
            }
            mActionSlot++;
        }
        mActionPhase = 1; mActionSlot = 0;
    }
    if (mActionPhase == 1) {
        while (mActionSlot < 9) {
            ItemStack* item = container->getItem(mActionSlot);
            if (item && item->mItem) {
                csc->handleAutoPlace("hotbar_items", mActionSlot);
                mLastActionTime = NOW; mActionSlot++; return;
            }
            mActionSlot++;
        }
        NotifyUtils::notify("Stored all items!", 2.f, Notification::Type::Info);
        resetAction();
    }
}

// ==================== SORT KEY ====================

int ContainerActions::getSortKey(ItemStack* item) {
    if (!item || !item->mItem) return 999999;
    Item* raw = item->getItem(); if (!raw) return 999999;
    std::string name = raw->mName;
    int cat = 13;
    if      (name.find("helmet")     !=std::string::npos) cat=0;
    else if (name.find("chestplate") !=std::string::npos) cat=1;
    else if (name.find("leggings")   !=std::string::npos) cat=2;
    else if (name.find("boots")      !=std::string::npos) cat=3;
    else if (name.find("sword")      !=std::string::npos) cat=4;
    else if (name.find("pickaxe")    !=std::string::npos) cat=5;
    else if (name.find("_axe")       !=std::string::npos) cat=6;
    else if (name.find("shovel")     !=std::string::npos) cat=7;
    else if (name.find("bow")!=std::string::npos && name.find("bowl")==std::string::npos) cat=8;
    else if (name.find("crossbow")   !=std::string::npos) cat=8;
    else if (name.find("shield")     !=std::string::npos) cat=9;
    else if (name.find("totem")      !=std::string::npos) cat=9;
    else if (name.find("apple")!=std::string::npos||name.find("cooked")!=std::string::npos||
             name.find("bread")!=std::string::npos||name.find("steak")!=std::string::npos) cat=10;
    else if (name.find("potion")!=std::string::npos||name.find("arrow")!=std::string::npos) cat=11;
    else if (item->mBlock) cat=12;

    int tier=0;
    if      (name.find("netherite")!=std::string::npos) tier=50;
    else if (name.find("diamond")  !=std::string::npos) tier=40;
    else if (name.find("iron")     !=std::string::npos) tier=30;
    else if (name.find("golden")!=std::string::npos||name.find("gold")!=std::string::npos) tier=20;
    else if (name.find("chain")    !=std::string::npos) tier=15;
    else if (name.find("stone")    !=std::string::npos) tier=10;
    else if (name.find("wooden")   !=std::string::npos) tier=5;

    int ench = 0;
    for (auto& [id,lvl] : item->gatherEnchants()) ench += lvl;
    return cat*10000 - tier*100 - ench;
}

// ==================== COMPUTE SWAP PAIRS ====================

void ContainerActions::computeSortSwaps(Actor* player) {
    mSortSwaps.clear(); mSortSwapIdx = 0; mSortSubStep = 0;
    auto cmm = player->getContainerManagerModel(); if (!cmm) return;

    struct SK { int slot; int key; };
    std::vector<SK> items; items.reserve(mChestSize);
    for (int i = 0; i < mChestSize; i++) {
        int key = 999999;
        TRY_CALL([&]() { key = getSortKey(cmm->getSlot(i)); });
        items.push_back({i, key});
    }

    // Selection sort → записываем пары свапов
    for (int i = 0; i < (int)items.size(); i++) {
        int minIdx = i;
        for (int j = i+1; j < (int)items.size(); j++)
            if (items[j].key < items[minIdx].key) minIdx = j;
        if (minIdx != i) {
            mSortSwaps.push_back({items[i].slot, items[minIdx].slot});
            std::swap(items[i], items[minIdx]);
        }
    }
}

// ==================== SORT CHEST (main dispatcher) ====================

void ContainerActions::processSortChest(ContainerScreenController* csc, Actor* player) {
    auto cmm = player->getContainerManagerModel();
    if (!cmm) { resetAction(); return; }

    // ============================
    // Phase 0: Инициализация
    // ============================
    if (mActionPhase == 0) {
        // Определяем размер сундука
        mChestSize = 27;
        TRY_CALL([&]() {
            for (int i = 27; i < 54; i++) {
                ItemStack* t = cmm->getSlot(i);
                if (t && t->mItem) { mChestSize = 54; break; }
            }
        });

        int chestItems = countChestItems(player);
        if (chestItems == 0) {
            NotifyUtils::notify("Chest is empty!", 2.f, Notification::Type::Info);
            resetAction(); return;
        }

        int freeSlots = countFreeInvSlots(player);

        if (freeSlots >= chestItems) {
            // === Достаточно места → режим через инвентарь ===
            mSortUseInPlace = false;
            mActionPhase = 1;
            mActionSlot = 0;
            NotifyUtils::notify("Sorting via inventory...", 1.5f, Notification::Type::Info);
        } else {
            // === Мало места → сортируем прямо в сундуке ===
            mSortUseInPlace = true;
            computeSortSwaps(player);
            if (mSortSwaps.empty()) {
                NotifyUtils::notify("Already sorted!", 2.f, Notification::Type::Info);
                resetAction(); return;
            }
            mActionPhase = 4;
            NotifyUtils::notify("Sorting in-place (" + std::to_string(mSortSwaps.size()) +
                                " swaps)...", 1.5f, Notification::Type::Info);
        }

        mLastActionTime = NOW;
        return;
    }

    uint64_t delay = std::max(static_cast<uint64_t>(mActionDelay.mValue),
                              static_cast<uint64_t>(50));

    // ============================
    // Phases 1-3: Via Inventory
    // ============================

    // Phase 1: Забираем всё из сундука
    if (mActionPhase == 1) {
        if (NOW - mLastActionTime < delay) return;

        while (mActionSlot < mChestSize) {
            bool found = false;
            TRY_CALL([&]() {
                ItemStack* item = cmm->getSlot(mActionSlot);
                if (item && item->mItem) {
                    csc->handleAutoPlace("container_items", mActionSlot);
                    mLastActionTime = NOW;
                    found = true;
                }
            });
            mActionSlot++;
            if (found) return;
        }

        // Всё забрали → вычисляем порядок
        mActionPhase = 2;
        mActionSlot = 0;
        // Сразу переходим к phase 2 (мгновенный)
    }

    // Phase 2: Вычисляем отсортированный порядок
    if (mActionPhase == 2) {
        auto supplies = player->getSupplies();
        if (!supplies) { resetAction(); return; }
        auto container = supplies->getContainer();
        if (!container) { resetAction(); return; }

        struct SortEntry {
            std::string collection;
            int collectionSlot;
            int sortKey;
        };
        std::vector<SortEntry> entries;

        for (int i = 0; i < 27; i++) {
            TRY_CALL([&]() {
                ItemStack* item = container->getItem(i + 9);
                if (item && item->mItem)
                    entries.push_back({"inventory_items", i, getSortKey(item)});
            });
        }
        for (int i = 0; i < 9; i++) {
            TRY_CALL([&]() {
                ItemStack* item = container->getItem(i);
                if (item && item->mItem)
                    entries.push_back({"hotbar_items", i, getSortKey(item)});
            });
        }

        std::sort(entries.begin(), entries.end(), [](const SortEntry& a, const SortEntry& b) {
            return a.sortKey < b.sortKey;
        });

        mSortOrder.clear();
        for (auto& e : entries)
            mSortOrder.push_back({e.collection, e.collectionSlot});

        if (mSortOrder.empty()) {
            NotifyUtils::notify("Nothing to sort!", 2.f, Notification::Type::Info);
            resetAction(); return;
        }

        mSortOrderIdx = 0;
        mActionPhase = 3;
        mLastActionTime = NOW;
        return;
    }

    // Phase 3: Кладём обратно в отсортированном порядке
    if (mActionPhase == 3) {
        if (NOW - mLastActionTime < delay) return;

        if (mSortOrderIdx >= (int)mSortOrder.size()) {
            NotifyUtils::notify("Sorted! (" + std::to_string(mSortOrder.size()) +
                                " items)", 2.f, Notification::Type::Info);
            resetAction(); return;
        }

        auto& entry = mSortOrder[mSortOrderIdx];
        TRY_CALL([&]() {
            csc->handleAutoPlace(entry.collection, entry.collectionSlot);
        });
        mSortOrderIdx++;
        mLastActionTime = NOW;
        return;
    }

    // ============================
    // Phase 4: In-Place Sort
    // ============================
    if (mActionPhase == 4) {
        // Увеличенный delay для in-place (безопаснее)
        uint64_t ipDelay = std::max(delay, static_cast<uint64_t>(100));
        if (NOW - mLastActionTime < ipDelay) return;

        if (mSortSwapIdx >= (int)mSortSwaps.size()) {
            NotifyUtils::notify("Sorted! (" + std::to_string(mSortSwaps.size()) +
                                " swaps)", 2.f, Notification::Type::Info);
            resetAction(); return;
        }

        auto& swap = mSortSwaps[mSortSwapIdx];
        std::string col = "container_items";

        switch (mSortSubStep) {
        case 0:
            // Поднимаем предмет из slotA в курсор
            TRY_CALL([&]() {
                csc->_tryBeginTransition(2, 0xFFFFFFFF, col, swap.a);
            });
            mSortSubStep = 1;
            mLastActionTime = NOW;
            break;

        case 1:
            // Кладём курсор в slotB — если B занят, они свапнутся
            // Пробуем targetState=2 (swap: cursor↔slot)
            TRY_CALL([&]() {
                csc->_tryBeginTransition(2, 0xFFFFFFFF, col, swap.b);
            });
            mSortSubStep = 2;
            mLastActionTime = NOW;
            break;

        case 2:
            // Кладём то что в курсоре обратно в slotA
            // targetState=0 (place: cursor→slot, стать idle)
            TRY_CALL([&]() {
                csc->_tryBeginTransition(0, 0xFFFFFFFF, col, swap.a);
            });
            mSortSwapIdx++;
            mSortSubStep = 0;
            mLastActionTime = NOW;
            break;
        }
    }
}