#include "ClickTp.hpp"

#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <SDK/Minecraft/World/Block.hpp>
#include <SDK/Minecraft/World/BlockLegacy.hpp>
#include <SDK/Minecraft/World/BlockSource.hpp>
#include <SDK/Minecraft/World/Level.hpp>
#include <SDK/Minecraft/World/HitResult.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

const char* CTGetKeyName(int vk)
{
    static char buf[32];
    switch (vk) {
        case 0: return "None"; case VK_LBUTTON: return "LMB"; case VK_RBUTTON: return "RMB";
        case VK_MBUTTON: return "MMB"; case VK_BACK: return "Back"; case VK_TAB: return "Tab";
        case VK_RETURN: return "Enter"; case VK_SHIFT: return "Shift"; case VK_CONTROL: return "Ctrl";
        case VK_MENU: return "Alt"; case VK_ESCAPE: return "Esc"; case VK_SPACE: return "Space";
        case VK_DELETE: return "Del"; case VK_INSERT: return "Ins"; case VK_HOME: return "Home";
        case VK_END: return "End"; default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buf, 32, "F%d", vk - VK_F1 + 1); return buf; }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { snprintf(buf, 32, "Num%d", vk - VK_NUMPAD0); return buf; }
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) { snprintf(buf, 32, "%c", (char)vk); return buf; }
    snprintf(buf, 32, "0x%02X", vk);
    return buf;
}

static bool isAnyKeyHeld()
{
    for (int vk = 8; vk <= 254; vk++) {
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;
        if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL) continue;
        if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN || vk == VK_RWIN) continue;
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
        if (vk == VK_ESCAPE) continue;
        if (GetAsyncKeyState(vk) & 0x8000) return true;
    }
    return false;
}

static int findHeldKey()
{
    for (int vk = 8; vk <= 254; vk++) {
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;
        if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LCONTROL || vk == VK_RCONTROL) continue;
        if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN || vk == VK_RWIN) continue;
        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
        if (vk == VK_ESCAPE) continue;
        if (GetAsyncKeyState(vk) & 0x8000) return vk;
    }
    return -1;
}

// =========================================================
// SERIALIZATION
// =========================================================
nlohmann::json ClickTp::serializeCustomData()
{
    std::lock_guard<std::mutex> lock(mMutex);

    nlohmann::json j;
    j["crosshairKey"] = mCrosshairKey;
    j["savePointKey"] = mSavePointKey;

    j["points"] = nlohmann::json::array();
    for (auto& pt : mPoints) {
        float px = pt.position.x, py = pt.position.y, pz = pt.position.z;
        if (std::isnan(px) || std::isinf(px)) px = 0.f;
        if (std::isnan(py) || std::isinf(py)) py = 0.f;
        if (std::isnan(pz) || std::isinf(pz)) pz = 0.f;

        nlohmann::json pj;
        pj["name"] = pt.name;
        pj["x"] = px;
        pj["y"] = py;
        pj["z"] = pz;
        pj["keybind"] = pt.keybind;
        pj["index"] = pt.index;
        j["points"].push_back(pj);
    }

    return j;
}

void ClickTp::deserializeCustomData(const nlohmann::json& j)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (j.is_null() || j.empty()) {
        mCrosshairKey = 'T';
        mSavePointKey = 'G';
        mPoints.clear();
        return;
    }

    if (j.contains("crosshairKey") && j["crosshairKey"].is_number())
        mCrosshairKey = j["crosshairKey"].get<int>();
    else
        mCrosshairKey = 'T';

    if (j.contains("savePointKey") && j["savePointKey"].is_number())
        mSavePointKey = j["savePointKey"].get<int>();
    else
        mSavePointKey = 'G';

    mPoints.clear();
    if (j.contains("points") && j["points"].is_array()) {
        for (auto& pj : j["points"]) {
            if (!pj.is_object()) continue;
            SavedPoint pt;
            pt.name = pj.value("name", "Point");
            pt.position.x = pj.value("x", 0.f);
            pt.position.y = pj.value("y", 0.f);
            pt.position.z = pj.value("z", 0.f);
            pt.keybind = pj.value("keybind", 0);
            pt.index = pj.value("index", 0);
            mPoints.push_back(pt);
        }
    }
}

// =========================================================
void ClickTp::onEnable()
{
    gFeatureManager->mDispatcher->listen<BaseTickEvent, &ClickTp::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent, &ClickTp::onPacketOutEvent, nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<PacketInEvent, &ClickTp::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent, &ClickTp::onRenderEvent>(this);

    auto player = ClientInstance::get()->getLocalPlayer();
    if (player) {
        auto r = player->getActorRotationComponent();
        if (r) mRots = { r->mPitch, r->mYaw, r->mYaw };
    }

    mKeyStates.clear();
    mIsBindingKey = mIsBindingCrosshairKey = mIsBindingSaveKey = false;
    mBindingPointIndex = -1;
    mBindWaitRelease = false;
    mRenamingIndex = -1;

    if (!mHudRegistered) {
        mPanelElement.mParentTypeIdentifier = sHudId;
        mPanelElement.mPos = { 10, 200 };
        mPanelElement.mSize = { 310, 200 };
        mPanelElement.mVisible = true;
        auto he = gFeatureManager->mModuleManager->getModule<HudEditor>();
        if (he) { he->registerElement(&mPanelElement); mHudRegistered = true; }
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mPacketPositions.clear();
}

void ClickTp::onDisable()
{
    gFeatureManager->mDispatcher->deafen<BaseTickEvent, &ClickTp::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent, &ClickTp::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketInEvent, &ClickTp::onPacketInEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent, &ClickTp::onRenderEvent>(this);
    mIsBindingKey = mIsBindingCrosshairKey = mIsBindingSaveKey = false;
    mRenamingIndex = -1;
    std::lock_guard<std::mutex> lock(mMutex);
    mPacketPositions.clear();
}

bool ClickTp::isKeyJustPressed(int vk) {
    if (vk <= 0) return false;
    bool down = GetAsyncKeyState(vk) & 0x8000;
    bool was = mKeyStates[vk];
    mKeyStates[vk] = down;
    return down && !was;
}

int ClickTp::getNextIndex() {
    int m = 0;
    for (auto& p : mPoints) if (p.index > m) m = p.index;
    return m + 1;
}

glm::vec3 ClickTp::findTarget() {
    auto p = ClientInstance::get()->getLocalPlayer();
    if (!p) return glm::vec3(FLT_MAX);
    HitResult* h = p->getLevel()->getHitResult();
    if (h && h->mType == HitType::BLOCK) return glm::vec3(h->mBlockPos);
    return extendedRaytrace(9999.f);
}

glm::vec3 ClickTp::extendedRaytrace(float maxRange) {
    auto p = ClientInstance::get()->getLocalPlayer();
    auto s = ClientInstance::get()->getBlockSource();
    if (!p || !s) return glm::vec3(FLT_MAX);
    auto r = p->getActorRotationComponent();
    if (!r) return glm::vec3(FLT_MAX);
    float pr = r->mPitch*(PI/180.f), yr = r->mYaw*(PI/180.f);
    glm::vec3 d = {-sinf(yr)*cosf(pr),-sinf(pr),cosf(yr)*cosf(pr)};
    glm::vec3 e = *p->getPos();
    glm::ivec3 last = {INT_MAX,INT_MAX,INT_MAX};
    for (float t = 0.5f; t <= maxRange; t += 0.5f) {
        glm::ivec3 bp = glm::ivec3(glm::floor(e+d*t));
        if (bp == last) continue; last = bp;
        if (bp.y < -64 || bp.y > 319) continue;
        Block* b = s->getBlock(bp);
        if (!b || b->mLegacy->isAir()) continue;
        int id = b->mLegacy->getBlockId();
        if (id >= 8 && id <= 11) continue;
        return glm::vec3(bp);
    }
    return glm::vec3(FLT_MAX);
}

glm::vec3 ClickTp::getTPDestination(glm::vec3 bp) {
    return {bp.x+0.5f, bp.y+1.f+mYOffset.mValue+PLAYER_HEIGHT, bp.z+0.5f};
}

std::shared_ptr<MovePlayerPacket> ClickTp::createPacketForPos(glm::vec3 pos) {
    auto p = ClientInstance::get()->getLocalPlayer();
    auto pk = MinecraftPackets::createPacket<MovePlayerPacket>();
    pk->mPos=pos; pk->mPlayerID=p->getRuntimeID();
    pk->mRot={mRots.x,mRots.y}; pk->mYHeadRot=mRots.z;
    pk->mResetPosition=PositionMode::Teleport; pk->mOnGround=true;
    pk->mRidingID=-1; pk->mCause=TeleportationCause::Unknown;
    pk->mSourceEntityType=ActorType::Player; pk->mTick=0;
    return pk;
}

void ClickTp::straightLineTP(glm::vec3 from, glm::vec3 to, bool save) {
    auto snd = ClientInstance::get()->getPacketSender();
    if (!snd) return;
    float st = mStepDistance.mValue;
    if (glm::length(to-from) < 0.01f) { snd->sendToServer(createPacketForPos(to).get()); return; }
    glm::vec3 dir = glm::normalize(to-from), cur = from;
    std::vector<glm::vec3> pts;
    while (glm::distance(cur,to) > st) { cur+=dir*st; pts.push_back(cur); snd->sendToServer(createPacketForPos(cur).get()); }
    pts.push_back(to); snd->sendToServer(createPacketForPos(to).get());
    if (save) { std::lock_guard<std::mutex> lk(mMutex); mPacketPositions=pts; mLastPathTime=NOW; }
}

void ClickTp::teleportTo(glm::vec3 dest) {
    auto p = ClientInstance::get()->getLocalPlayer();
    if (!p) return;
    straightLineTP(*p->getPos(), dest, true);
    p->setPosition(dest);
    auto sv = p->getStateVectorComponent();
    if (sv) sv->mVelocity = glm::vec3(0.f);
}

// =========================================================
// MAIN TICK
// =========================================================
void ClickTp::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;

    bool cursorShowing = ClientInstance::get()->getMouseGrabbed();

    // =====================================================
    // БИНДИНГ — работает ВСЕГДА
    // =====================================================
    if (mIsBindingKey || mIsBindingCrosshairKey || mIsBindingSaveKey)
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            mIsBindingKey = mIsBindingCrosshairKey = mIsBindingSaveKey = false;
            mBindingPointIndex = -1;
            mBindWaitRelease = false;
            return;
        }

        if (mBindWaitRelease) {
            if (!isAnyKeyHeld()) mBindWaitRelease = false;
            return;
        }

        int pressed = findHeldKey();
        if (pressed > 0) {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mIsBindingKey && mBindingPointIndex >= 0 && mBindingPointIndex < (int)mPoints.size()) {
                for (auto& p : mPoints) if (p.keybind == pressed) p.keybind = 0;
                mPoints[mBindingPointIndex].keybind = pressed;
            }
            else if (mIsBindingCrosshairKey) mCrosshairKey = pressed;
            else if (mIsBindingSaveKey) mSavePointKey = pressed;

            mIsBindingKey = mIsBindingCrosshairKey = mIsBindingSaveKey = false;
            mBindingPointIndex = -1;
            mBindWaitRelease = false;
            mKeyStates[pressed] = true;
        }
        return;
    }

    // =====================================================
    // КЛАВИШИ ТП — ТОЛЬКО В ИГРЕ
    // =====================================================
    if (cursorShowing) return;
    if (mRenamingIndex >= 0) return;

    if (isKeyJustPressed(mCrosshairKey)) {
        glm::vec3 hit = findTarget();
        if (hit.x != FLT_MAX) teleportTo(getTPDestination(hit));
    }

    if (isKeyJustPressed(mSavePointKey)) {
        std::lock_guard<std::mutex> lock(mMutex);
        SavedPoint pt;
        int idx = getNextIndex();
        pt.name = "Point " + std::to_string(idx);
        pt.position = *player->getPos();
        pt.keybind = 0; pt.index = idx;
        mPoints.push_back(pt);
    }

    glm::vec3 bindTpDest(FLT_MAX);
    {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto& pt : mPoints) {
            if (pt.keybind > 0 && isKeyJustPressed(pt.keybind)) {
                bindTpDest = pt.position;
                break;
            }
        }
    }
    // Телепорт БЕЗ мьютекса — не будет deadlock
    if (bindTpDest.x != FLT_MAX) {
        teleportTo(bindTpDest);
    }
}

void ClickTp::onPacketOutEvent(PacketOutEvent& event) {
    if (event.mPacket->getId() == PacketID::MovePlayer) {
        auto pkt = event.getPacket<MovePlayerPacket>();
        mRots = {pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot};
    }
}

void ClickTp::onPacketInEvent(PacketInEvent& event) {}

// =========================================================
// RENDER
// =========================================================
void ClickTp::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    auto drawList = ImGui::GetBackgroundDrawList();

    bool cursorShowing = ClientInstance::get()->getMouseGrabbed();
    float sc = mScale.mValue / 100.f;

    // === TP Path ===
    if (mDrawPath.mValue) {
        std::lock_guard<std::mutex> lk(mMutex);
        uint64_t now = NOW; float alpha = 1.f;
        if (mLastPathTime+1500 < now) mPacketPositions.clear();
        else alpha = std::clamp(1.f-float(now-mLastPathTime)/1500.f, 0.f, 1.f);
        if (!mPacketPositions.empty()) {
            std::vector<ImVec2> pts;
            for (auto& p : mPacketPositions) { ImVec2 s; if (RenderUtils::worldToScreen(p,s)) pts.push_back(s); }
            for (size_t i = 0; i+1 < pts.size(); i++) {
                ImColor c = ColorUtils::getThemedColor((float)i*0.05f); c.Value.w *= alpha;
                drawList->AddLine(pts[i], pts[i+1], c, 2.f);
            }
        }
    }

    // === Копируем mPoints под мьютексом для рендера ===
    std::vector<SavedPoint> pointsCopy;
    {
        std::lock_guard<std::mutex> lk(mMutex);
        pointsCopy = mPoints;
    }

    // === 3D markers ===
    if (mShowPoints.mValue) {
        for (auto& pt : pointsCopy) {
            glm::vec3 feet = pt.position - glm::vec3(0,PLAYER_HEIGHT,0);
            AABB box; box.mMin=feet-glm::vec3(.25f,0,.25f); box.mMax=feet+glm::vec3(.25f,1.8f,.25f);
            auto bp = MathUtils::getImBoxPoints(box);
            if (!bp.empty()) {
                drawList->AddConvexPolyFilled(bp.data(),bp.size(),ImColor(100,100,255,30));
                drawList->AddPolyline(bp.data(),bp.size(),ImColor(100,100,255,150),true,2.f);
            }
            ImVec2 sp;
            if (RenderUtils::worldToScreen(feet+glm::vec3(0,2.2f,0), sp)) {
                std::string lbl = pt.name;
                if (pt.keybind > 0) lbl += " [" + std::string(CTGetKeyName(pt.keybind)) + "]";
                float dist = glm::distance(*player->getPos(), pt.position);
                char db[16]; snprintf(db,16," %.0fm",dist); lbl += db;
                ImVec2 ts = ImGui::CalcTextSize(lbl.c_str());
                sp.x -= ts.x*0.5f;
                drawList->AddRectFilled({sp.x-3,sp.y-2},{sp.x+ts.x+3,sp.y+ts.y+2},IM_COL32(0,0,0,140),3.f);
                drawList->AddText(sp, ImColor(180,180,255,255), lbl.c_str());
            }
        }
    }

    if (!mShowPoints.mValue) return;

    // === PANEL ===
    ImVec2 panelPos = mPanelElement.getPos();
    float px = panelPos.x, py = panelPos.y, pw = 310*sc;
    float lineH = 22*sc, headerH = 26*sc, bindW = 55*sc, delW = 20*sc;
    float fontSize = ImGui::GetFontSize() * sc;

    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool canClick = cursorShowing && ImGui::GetIO().MouseClicked[0];

    float totalH = headerH + 4*sc + 30*sc + 30*sc + 8*sc;
    totalH += pointsCopy.empty() ? 24*sc : (pointsCopy.size()*lineH + 4*sc);
    mPanelElement.mSize = {pw, totalH};

    // BG
    drawList->AddRectFilled({px-4*sc,py-4*sc},{px+pw+4*sc,py+totalH+4*sc},IM_COL32(15,15,20,200),6*sc);
    drawList->AddRect({px-4*sc,py-4*sc},{px+pw+4*sc,py+totalH+4*sc},IM_COL32(80,80,120,150),6*sc,0,1.5f);

    // Header
    drawList->AddRectFilled({px-4*sc,py-4*sc},{px+pw+4*sc,py+headerH},IM_COL32(40,40,60,220),6*sc);
    const char* title = "ClickTp Waypoints";
    ImVec2 tts = ImGui::CalcTextSize(title);
    drawList->AddText(ImGui::GetFont(),fontSize,{px+(pw-tts.x*sc)*0.5f,py+(headerH-tts.y*sc)*0.5f-2*sc},ImColor(200,200,255,255),title);

    float curY = py + headerH + 4*sc;

    auto drawKeyBtn = [&](const char* label, int& key, bool& binding, float y) {
        drawList->AddText(ImGui::GetFont(),fontSize,{px+4*sc,y+4*sc},IM_COL32(180,180,180,255),label);
        float bx = px+pw-bindW-4*sc;
        ImVec2 mn(bx,y+2*sc), mx(bx+bindW,y+24*sc);
        bool hv = mouse.x>=mn.x&&mouse.x<=mx.x&&mouse.y>=mn.y&&mouse.y<=mx.y;
        std::string kt = binding ? "Press key..." : CTGetKeyName(key);
        ImU32 bc = binding ? IM_COL32(200,100,0,200) : (hv ? IM_COL32(70,70,100,200) : IM_COL32(50,50,70,200));
        drawList->AddRectFilled(mn,mx,bc,4*sc);
        drawList->AddRect(mn,mx,IM_COL32(100,100,140,200),4*sc);
        ImVec2 ks = ImGui::CalcTextSize(kt.c_str());
        drawList->AddText(ImGui::GetFont(),fontSize,{bx+(bindW-ks.x*sc)*0.5f,y+(24*sc-ks.y*sc)*0.5f+2*sc},IM_COL32(255,255,255,255),kt.c_str());
        if (hv && canClick && !mIsBindingKey) { binding = !binding; if (binding) mBindWaitRelease = true; }
    };

    drawKeyBtn("Crosshair TP:", mCrosshairKey, mIsBindingCrosshairKey, curY); curY += 30*sc;
    drawKeyBtn("Save Point:", mSavePointKey, mIsBindingSaveKey, curY); curY += 30*sc;

    drawList->AddLine({px,curY},{px+pw,curY},IM_COL32(80,80,120,100)); curY += 8*sc;

    if (pointsCopy.empty()) {
        drawList->AddText(ImGui::GetFont(),fontSize,{px+4*sc,curY+2*sc},IM_COL32(120,120,120,200),"No saved points");
    } else {
        int deleteIdx = -1;
        for (int i = 0; i < (int)pointsCopy.size(); i++) {
            auto& pt = pointsCopy[i];
            float ry = curY + i*lineH;
            if (i%2==0) drawList->AddRectFilled({px,ry},{px+pw,ry+lineH},IM_COL32(30,30,45,100));

            float dist = glm::distance(*player->getPos(), pt.position);
            char nb[96]; snprintf(nb,96,"%s (%.0fm)", pt.name.c_str(), dist);
            float neX = px+4*sc+ImGui::CalcTextSize(nb).x*sc;
            bool nH = mouse.x>=px+4*sc&&mouse.x<=neX&&mouse.y>=ry&&mouse.y<=ry+lineH;
            drawList->AddText(ImGui::GetFont(),fontSize,{px+4*sc,ry+3*sc}, nH?IM_COL32(255,255,150,255):IM_COL32(220,220,255,255), nb);
            if (nH && canClick) {
                mRenamingIndex = i;
                strncpy(mRenameBuffer,pt.name.c_str(),sizeof(mRenameBuffer)-1);
                mRenameBuffer[sizeof(mRenameBuffer)-1] = '\0';
                mRenameJustOpened = true;
            }

            float bbx = px+pw-bindW-delW-10*sc;
            ImVec2 bmn(bbx,ry+1*sc), bmx(bbx+bindW,ry+lineH-1*sc);
            bool bH = mouse.x>=bmn.x&&mouse.x<=bmx.x&&mouse.y>=bmn.y&&mouse.y<=bmx.y;
            bool isB = mIsBindingKey && mBindingPointIndex==i;
            std::string bt = isB ? "Press key..." : (pt.keybind>0 ? CTGetKeyName(pt.keybind) : "Bind");
            ImU32 bc2 = isB ? IM_COL32(200,100,0,200) : (bH ? IM_COL32(60,90,60,200) : IM_COL32(40,60,40,200));
            drawList->AddRectFilled(bmn,bmx,bc2,3*sc);
            drawList->AddRect(bmn,bmx,IM_COL32(80,120,80,200),3*sc);
            ImVec2 bts = ImGui::CalcTextSize(bt.c_str());
            drawList->AddText(ImGui::GetFont(),fontSize,{bbx+(bindW-bts.x*sc)*0.5f,ry+(lineH-bts.y*sc)*0.5f},IM_COL32(200,255,200,255),bt.c_str());
            if (bH && canClick && !mIsBindingCrosshairKey && !mIsBindingSaveKey) {
                if (isB) { mIsBindingKey=false; mBindingPointIndex=-1; }
                else { mIsBindingKey=true; mBindingPointIndex=i; mBindWaitRelease=true; }
            }

            float dx = px+pw-delW-4*sc;
            ImVec2 dmn(dx,ry+2*sc), dmx(dx+delW,ry+lineH-2*sc);
            bool dH = mouse.x>=dmn.x&&mouse.x<=dmx.x&&mouse.y>=dmn.y&&mouse.y<=dmx.y;
            drawList->AddRectFilled(dmn,dmx,dH?IM_COL32(180,50,50,200):IM_COL32(100,30,30,150),3*sc);
            ImVec2 xs = ImGui::CalcTextSize("X");
            drawList->AddText(ImGui::GetFont(),fontSize,{dx+(delW-xs.x*sc)*0.5f,ry+(lineH-xs.y*sc)*0.5f},IM_COL32(255,100,100,255),"X");
            if (dH && canClick) deleteIdx = i;
        }
        if (deleteIdx >= 0) {
            std::lock_guard<std::mutex> lk(mMutex);
            if (deleteIdx < (int)mPoints.size()) {
                mPoints.erase(mPoints.begin()+deleteIdx);
                if (mBindingPointIndex==deleteIdx) { mIsBindingKey=false; mBindingPointIndex=-1; }
                else if (mBindingPointIndex>deleteIdx) mBindingPointIndex--;
                if (mRenamingIndex==deleteIdx) mRenamingIndex=-1;
                else if (mRenamingIndex>deleteIdx) mRenamingIndex--;
            }
        }
    }

    // Rename popup
    if (mRenamingIndex >= 0 && mRenamingIndex < (int)pointsCopy.size()) {
        ImGui::SetNextWindowSize({250,70});
        ImGui::SetNextWindowPos({px+pw+10,py+headerH});
        if (ImGui::Begin("##CTRename",nullptr,
            ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar)) {
            if (mRenameJustOpened) { ImGui::SetKeyboardFocusHere(); mRenameJustOpened=false; }
            ImGui::Text("Rename:"); ImGui::SameLine();
            if (ImGui::InputText("##rn",mRenameBuffer,sizeof(mRenameBuffer),ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (strlen(mRenameBuffer) > 0) {
                    std::lock_guard<std::mutex> lk(mMutex);
                    if (mRenamingIndex < (int)mPoints.size())
                        mPoints[mRenamingIndex].name = mRenameBuffer;
                }
                mRenamingIndex = -1;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) mRenamingIndex = -1;
        }
        ImGui::End();
    }

    // Binding overlay
    if (mIsBindingKey || mIsBindingCrosshairKey || mIsBindingSaveKey) {
        ImVec2 ss = ImGui::GetIO().DisplaySize;
        float cx = ss.x*0.5f, cy = ss.y*0.5f;
        const char* msg = mBindWaitRelease ? "Release all keys, then press desired key..." : "Press any key to bind (ESC = cancel)";
        ImVec2 ms = ImGui::CalcTextSize(msg);
        drawList->AddRectFilled({cx-ms.x*0.5f-15,cy-8},{cx+ms.x*0.5f+15,cy+ms.y+8},IM_COL32(200,100,0,220),8.f);
        drawList->AddRect({cx-ms.x*0.5f-15,cy-8},{cx+ms.x*0.5f+15,cy+ms.y+8},IM_COL32(255,150,50,255),8.f,0,2.f);
        drawList->AddText({cx-ms.x*0.5f,cy},IM_COL32(255,255,255,255),msg);
    }
}