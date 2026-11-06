#include "ArrowTP.hpp"
#include <Features/FeatureManager.hpp>
#include <SDK/Minecraft/ClientInstance.hpp>
#include <SDK/Minecraft/Actor/Actor.hpp>
#include <SDK/Minecraft/Actor/ActorType.hpp>
#include <SDK/Minecraft/Actor/Components/StateVectorComponent.hpp>
#include <SDK/Minecraft/Inventory/PlayerInventory.hpp>
#include <SDK/Minecraft/Inventory/Item.hpp>
#include <SDK/Minecraft/Inventory/ItemStack.hpp>
#include <SDK/Minecraft/Network/MinecraftPackets.hpp>
#include <SDK/Minecraft/Network/Packets/MovePlayerPacket.hpp>
#include <SDK/Minecraft/Network/LoopbackPacketSender.hpp>
#include <Utils/GameUtils/ActorUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>
#include <Utils/MiscUtils/RenderUtils.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>

void ArrowTP::onEnable() {
    gFeatureManager->mDispatcher->listen<BaseTickEvent,&ArrowTP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->listen<PacketOutEvent,&ArrowTP::onPacketOutEvent,nes::event_priority::ABSOLUTE_LAST>(this);
    gFeatureManager->mDispatcher->listen<RenderEvent,&ArrowTP::onRenderEvent>(this);
    auto p=ClientInstance::get()->getLocalPlayer();
    if(p){auto r=p->getActorRotationComponent();if(r)mRots={r->mPitch,r->mYaw,r->mYaw};}
    mLastBowRelease=0;mBowDrawing=false;mHeldBow=false;mDidTP=false;
    mPendingBack={};mGhostVisible=false;mTrackedArrows.clear();mStatus="Ready";
    {std::lock_guard<std::mutex>lk(mMutex);mPacketPositions.clear();}
}

void ArrowTP::onDisable() {
    gFeatureManager->mDispatcher->deafen<BaseTickEvent,&ArrowTP::onBaseTickEvent>(this);
    gFeatureManager->mDispatcher->deafen<PacketOutEvent,&ArrowTP::onPacketOutEvent>(this);
    gFeatureManager->mDispatcher->deafen<RenderEvent,&ArrowTP::onRenderEvent>(this);
    mPendingBack={};mGhostVisible=false;mTrackedArrows.clear();
    {std::lock_guard<std::mutex>lk(mMutex);mPacketPositions.clear();}
}

bool ArrowTP::isHoldingBow(Actor*p) {
    if(!p)return false;
    auto s=p->getSupplies();if(!s)return false;
    auto c=s->getContainer();if(!c)return false;
    auto i=c->getItem(s->mSelectedSlot);
    if(!i||!i->mItem)return false;
    std::string n=i->getItem()->getmName();
    return n.find("bow")!=std::string::npos||n.find("trident")!=std::string::npos;
}

std::shared_ptr<MovePlayerPacket> ArrowTP::createPacketForPos(glm::vec3 pos) {
    auto p=ClientInstance::get()->getLocalPlayer();
    auto pk=MinecraftPackets::createPacket<MovePlayerPacket>();
    pk->mPos=pos;pk->mPlayerID=p->getRuntimeID();
    pk->mRot={mRots.x,mRots.y};pk->mYHeadRot=mRots.z;
    pk->mResetPosition=PositionMode::Teleport;pk->mOnGround=true;
    pk->mRidingID=-1;pk->mCause=TeleportationCause::Unknown;
    pk->mSourceEntityType=ActorType::Player;pk->mTick=0;
    return pk;
}

void ArrowTP::straightLineTP(glm::vec3 from, glm::vec3 to, bool save) {
    auto snd=ClientInstance::get()->getPacketSender();if(!snd)return;
    float st=mStepDistance.mValue;
    if(glm::length(to-from)<0.01f){snd->sendToServer(createPacketForPos(to).get());return;}
    glm::vec3 dir=glm::normalize(to-from),cur=from;
    std::vector<glm::vec3>pts;
    while(glm::distance(cur,to)>st){cur+=dir*st;pts.push_back(cur);snd->sendToServer(createPacketForPos(cur).get());}
    pts.push_back(to);snd->sendToServer(createPacketForPos(to).get());
    if(save){std::lock_guard<std::mutex>lk(mMutex);mPacketPositions=pts;mLastPathTime=NOW;}
}

void ArrowTP::performTP(glm::vec3 dest) {
    auto p=ClientInstance::get()->getLocalPlayer();if(!p)return;
    glm::vec3 pp=*p->getPos();
    if(mTPBack.mValue){mPendingBack.originalPos=pp;mPendingBack.ticksRemaining=(int)mTPBackDelay.mValue;mPendingBack.active=true;}
    straightLineTP(pp,dest,true);
    p->setPosition(dest);
    auto sv=p->getStateVectorComponent();if(sv)sv->mVelocity=glm::vec3(0.f);
    mGhostPos=dest;mGhostVisible=true;mDidTP=true;
}

// =========================================================
// MAIN TICK
// =========================================================
void ArrowTP::onBaseTickEvent(BaseTickEvent& event)
{
    auto player = event.mActor;
    if (!player) return;
    uint64_t now = NOW;

    // === Bow detection ===
    bool isUsing = player->getStatusFlag(ActorFlags::Usingitem);
    if (isUsing && !mBowDrawing) { mBowDrawing = true; mHeldBow = isHoldingBow(player); }
    else if (!isUsing && mBowDrawing) {
        mBowDrawing = false;
        if (mHeldBow) { mLastBowRelease = now; mDidTP = false; mTrackedArrows.clear(); }
        mHeldBow = false;
    }

    // === Track window ===
    uint64_t windowMs = (uint64_t)(mTrackWindow.mValue * 1000.f);
    bool inWindow = mLastBowRelease > 0 && (now - mLastBowRelease) < windowMs;

    if (!inWindow || mDidTP) {
        if (!inWindow) { mStatus = mDidTP ? "Done" : "Ready (shoot bow)"; mTrackedArrows.clear(); }
        goto tpback;
    }

    // === Scan arrows ===
    {
        mStatus = "Tracking...";
        std::unordered_set<uint64_t> currentIds;
        bool doTP = false;
        glm::vec3 tpDest(0);

        try {
            auto actors = ActorUtils::getActorList(false, true);
            for (auto actor : actors) {
                if (!actor) continue;
                std::string name = actor->getRawName();
                if (name.find("arrow") == std::string::npos && name.find("trident") == std::string::npos) continue;

                uint64_t id = actor->getRuntimeID();
                glm::vec3 pos = *actor->getPos();
                currentIds.insert(id);

                auto it = mTrackedArrows.find(id);
                if (it == mTrackedArrows.end()) {
                    // Новая стрела — рядом с нами?
                    if (glm::distance(*player->getPos(), pos) < 8.f) {
                        TrackedArrow ta;
                        ta.spawnPos = pos; ta.lastPos = pos; ta.prevPos = pos;
                        mTrackedArrows[id] = ta;
                    }
                } else {
                    TrackedArrow& a = it->second;
                    a.aliveTicks++;
                    a.prevPos = a.lastPos;
                    a.lastPos = pos;

                    float distFromSpawn = glm::distance(a.spawnPos, pos);
                    if (distFromSpawn > a.maxDistFromSpawn) a.maxDistFromSpawn = distFromSpawn;

                    float move = glm::distance(a.prevPos, pos);
                    if (move > 0.3f) { a.hasFlown = true; a.stuckTicks = 0; }
                    else a.stuckTicks++;

                    // БЛОК: стрела застряла после полёта
                    if (a.hasFlown && a.stuckTicks >= 3 &&
                        a.maxDistFromSpawn >= mMinFlightDist.mValue &&
                        a.aliveTicks >= (int)mMinFlightTicks.mValue && !a.tpDone)
                    {
                        a.tpDone = true;
                        tpDest = a.lastPos; tpDest.y += PLAYER_HEIGHT;
                        doTP = true; mStatus = "Block hit! TPing...";
                    }
                }
            }
        } catch (...) {}

        if (doTP) { performTP(tpDest); mTrackedArrows.clear(); }
        else {
            // === ИСЧЕЗНУВШИЕ СТРЕЛЫ = попали в entity ===
            for (auto it = mTrackedArrows.begin(); it != mTrackedArrows.end(); ++it)
            {
                if (currentIds.count(it->first) != 0) continue;

                TrackedArrow& a = it->second;
                a.missedTicks++;

                // Стрела исчезла — если жила хотя бы 2 тика, ТП к последней позиции
                // Сервер удаляет entity стрелы когда она попадает в entity
                if (a.missedTicks >= 1 && a.aliveTicks >= 2 && !a.tpDone)
                {
                    a.tpDone = true;
                    tpDest = a.lastPos;
                    tpDest.y += PLAYER_HEIGHT;
                    mStatus = "Entity hit! TPing...";
                    performTP(tpDest);
                    mTrackedArrows.clear();
                    break;
                }
                // Стрела пропала слишком рано — не наша
                else if (a.missedTicks >= 5) {
                    mTrackedArrows.erase(it);
                    break;
                }
            }
        }

        // Debug status
        if (!mTrackedArrows.empty() && !mDidTP) {
            for (auto& pair : mTrackedArrows) {
                TrackedArrow& a = pair.second;
                char buf[128];
                snprintf(buf, sizeof(buf), "Track: dist=%.1f t=%d %s",
                    a.maxDistFromSpawn, a.aliveTicks, a.hasFlown ? "FLY" : "spawn");
                mStatus = buf;
                break;
            }
        }
    }

tpback:
    if (mPendingBack.active && mTPBack.mValue) {
        mPendingBack.ticksRemaining--;
        if (mPendingBack.ticksRemaining <= 0) {
            glm::vec3 cur = *player->getPos();
            straightLineTP(cur, mPendingBack.originalPos, true);
            player->setPosition(mPendingBack.originalPos);
            auto sv = player->getStateVectorComponent();
            if (sv) sv->mVelocity = glm::vec3(0.f);
            mPendingBack.active = false; mGhostVisible = false;
        }
    }
}

void ArrowTP::onPacketOutEvent(PacketOutEvent& event) {
    if (event.mPacket->getId() == PacketID::MovePlayer) {
        auto pkt = event.getPacket<MovePlayerPacket>();
        mRots = {pkt->mRot.x, pkt->mRot.y, pkt->mYHeadRot};
    }
}

void ArrowTP::onRenderEvent(RenderEvent& event)
{
    auto player = ClientInstance::get()->getLocalPlayer();
    if (!player) return;
    std::lock_guard<std::mutex> lk(mMutex);
    auto dl = ImGui::GetBackgroundDrawList();
    uint64_t now = NOW; float alpha = 1.f;
    if (mLastPathTime+1500 < now) mPacketPositions.clear();
    else alpha = std::clamp(1.f-float(now-mLastPathTime)/1500.f, 0.f, 1.f);

    if (mDrawPath.mValue && !mPacketPositions.empty()) {
        std::vector<ImVec2> pts;
        for (auto& p : mPacketPositions) { ImVec2 s; if (RenderUtils::worldToScreen(p,s)) pts.push_back(s); }
        for (size_t i = 0; i+1 < pts.size(); i++) {
            ImColor c = ColorUtils::getThemedColor((float)i*0.05f); c.Value.w *= alpha;
            dl->AddLine(pts[i], pts[i+1], c, 2.f);
        }
    }

    if (mDrawGhost.mValue && mGhostVisible && alpha > 0.01f) {
        glm::vec3 f = mGhostPos-glm::vec3(0,PLAYER_HEIGHT,0);
        AABB b; b.mMin=f-glm::vec3(.3f,0,.3f); b.mMax=f+glm::vec3(.3f,1.8f,.3f);
        auto bp = MathUtils::getImBoxPoints(b);
        if (!bp.empty()) {
            ImColor fl = ColorUtils::getThemedColor(0); fl.Value.w = 0.2f*alpha;
            ImColor ol = ColorUtils::getThemedColor(0); ol.Value.w = 0.7f*alpha;
            dl->AddConvexPolyFilled(bp.data(),bp.size(),fl);
            dl->AddPolyline(bp.data(),bp.size(),ol,true,2.f);
        }
    }

    // Status
    ImVec2 ss = ImGui::GetIO().DisplaySize;
    float cx = ss.x*0.5f, ty = ss.y*0.25f;
    ImVec2 ts = ImGui::CalcTextSize(mStatus.c_str());
    dl->AddRectFilled({cx-ts.x*0.5f-8,ty-4},{cx+ts.x*0.5f+8,ty+ts.y+4},IM_COL32(0,0,0,120),4.f);
    ImColor tc = mBowDrawing ? ImColor(255,200,50,230) :
        (!mTrackedArrows.empty() ? ImColor(100,255,100,230) : ImColor(180,180,180,200));
    dl->AddText({cx-ts.x*0.5f,ty}, tc, mStatus.c_str());

    if (mPendingBack.active && mTPBack.mValue) {
        float ty2 = ss.y*0.30f;
        std::string bt = "TP Back: "+std::to_string(mPendingBack.ticksRemaining)+" ticks";
        ImVec2 ts2 = ImGui::CalcTextSize(bt.c_str());
        dl->AddRectFilled({cx-ts2.x*0.5f-8,ty2-4},{cx+ts2.x*0.5f+8,ty2+ts2.y+4},IM_COL32(0,0,0,120),4.f);
        dl->AddText({cx-ts2.x*0.5f,ty2}, ImColor(255,200,50,230), bt.c_str());
    }
}