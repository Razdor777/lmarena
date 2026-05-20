#pragma once
//
// Created by vastrakai on 6/24/2024.
//

#include <cstdint>
#include <future>
#include <include/libhat/include/libhat.hpp>
#include <include/libhat/include/libhat/Scanner.hpp>
#include <include/libhat/include/libhat/Signature.hpp>

enum class SigType { Sig, RefSig };

#define DEFINE_SIG(name, str, sig_type, offset)                                \
public:                                                                        \
  static inline uintptr_t name;                                                \
                                                                               \
private:                                                                       \
  static void name##_initializer() {                                           \
    auto result =                                                              \
        scanSig(hat::compile_signature<str>(), xorstr_(#name), offset);        \
    if (!result.has_result()) {                                                \
      name = 0;                                                                \
      return;                                                                  \
    }                                                                          \
    if (sig_type == SigType::Sig)                                              \
      name = reinterpret_cast<uintptr_t>(result.get());                        \
    else                                                                       \
      name = reinterpret_cast<uintptr_t>(result.rel(offset));                  \
  }                                                                            \
  static inline std::function<void()> name##_function =                        \
      (mSigInitializers.emplace_back(name##_initializer),                      \
       std::function<void()>());                                               \
                                                                               \
public:

class SigManager {
  static hat::scan_result scanSig(hat::signature_view sig,
                                  const std::string &name, int offset = 0);

  static inline std::vector<std::function<void()>> mSigInitializers;
  static inline int mSigScanCount;

public:
  static inline bool mIsInitialized = false;
  static inline std::unordered_map<std::string, uintptr_t> mSigs;

  DEFINE_SIG(Actor_setPosition,
             "48 89 ? ? ? 57 48 83 EC ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? 48 85 "
             "? 74 ? 48 8B ? 48 8B ? ? ? ? ? FF 15",
             SigType::Sig, 0);
  DEFINE_SIG(Actor_getNameTag,
             "E8 ? ? ? ? 48 83 78 18 ? 76 ? 48 8B ? EB ? 48 8D ? ? ? ? ? 4C 8B "
             "? 4C 8B ? 48 8D ? ? ? ? ? 48 8D ? ? ? E8 ? ? ? ? 48 8B",
             SigType::RefSig, 1); // TODO: Check this and make sure it works, as
                                  // a lot of the other references were inlined
  DEFINE_SIG(Actor_setNameTag,
             "E8 ? ? ? ? 4C 8D ? ? ? ? ? 8B D3 48 8B ? E8 ? ? ? ? E9",
             SigType::RefSig, 1);
  DEFINE_SIG(ActorRenderDispatcher_render,
             "E8 ? ? ? ? 8B 43 ? 89 44 ? ? 48 8D ? ? ? 48 8B ? ? E8 ? ? ? ? 48 "
             "85 ? 0F 84 ? ? ? ? F3 44",
             SigType::RefSig, 1);
  DEFINE_SIG(ClientInstance_mBgfx,
             "48 8B ? ? ? ? ? 48 8D ? ? ? ? ? FF 15 ? ? ? ? 0F B7",
             SigType::RefSig, 3);
  DEFINE_SIG(ClientInstance_grabMouse,
             "40 ? 48 83 EC ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 84 "
             "C0 74 ? 48 8B ? ? ? ? ? 48 8B ? 48 8B ? ? ? ? ? 48 83 C4 ? 5B 48 "
             "FF ? ? ? ? ? 48 83 C4 ? 5B C3 40",
             SigType::Sig, 0);
  DEFINE_SIG(ClientInstance_releaseMouse,
             "40 ? 48 83 EC ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 84 "
             "C0 74 ? 48 8B ? ? ? ? ? 48 8B ? 48 8B ? ? ? ? ? 48 83 C4 ? 5B 48 "
             "FF ? ? ? ? ? 48 83 C4 ? 5B C3 48 89",
             SigType::Sig, 0);
  DEFINE_SIG(ContainerScreenController_tick,
             "E8 ? ? ? ? 48 8B ? ? ? ? ? 48 8D ? ? ? ? ? 41 B8 ? ? ? ? 8B F8",
             SigType::RefSig, 1);
  DEFINE_SIG(ContainerScreenController_tryBeginTransition,
             "40 53 56 57 41 56 41 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 33 C4 48 "
             "89 44 24 ? 8B FA",
             SigType::Sig, 0);
  DEFINE_SIG(ContainerScreenController_handleAutoPlace,
             "E8 ? ? ? ? 66 ? ? ? ? ? ? ? 0F 8C", SigType::RefSig, 1);
  DEFINE_SIG(ContainerValidation_handleTransfer_jnz,
             "85 FF 75 ? 48 8B 4D ? 48 8B 01 41 8B D4 48 8B 40 38",
             SigType::Sig, 0);
  DEFINE_SIG(ComplexInventoryTransaction_vtable,
             "48 8D 05 ?? ?? ?? ?? 48 89 07 C7 47 08 03 00 00 00 48 8D 5F 10",
             SigType::RefSig, 3);
  DEFINE_SIG(
      isCriticalHit,
      "48 89 5C 24 ? 57 48 83 EC ? 8B 41 ? 48 8B FA 48 8B D9 89 44 24 ? 48 8B "
      "49 ? 48 8D 54 24 ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 0F 57 C0",
      SigType::Sig, 0);
  DEFINE_SIG(EnchantUtils_getEnchantLevel, "E8 ? ? ? ? 8B D8 4D 8B ? 49 8B",
             SigType::RefSig, 1);
  DEFINE_SIG(GameMode_getDestroyRate, "E8 ? ? ? ? 0F 28 ? 49 8B ? ? E8",
             SigType::RefSig, 1);
  DEFINE_SIG(ContainerValidation_handlePlace_jnz, "41 8B 5F 18 41 0F B6 47 1C",
             SigType::Sig, 0);
  DEFINE_SIG(ContainerValidation_handlePlace_test, "45 85 FF 74 44 41 83 FF 06",
             SigType::Sig, 0);
  DEFINE_SIG(
      HoverTextRenderer_render,
      "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 57 48 81 EC ? ? ? ? 0F 29 "
      "70 E8 0F 29 78 D8 44 0F 29 40 ? 49 8B D9 49 8B F8 48 8B F1 48 8B 6A 10",
      SigType::Sig, 0);
  DEFINE_SIG(GameMode_baseUseItem,
             "E8 ? ? ? ? 84 C0 74 ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? "
             "? ? 48 85",
             SigType::RefSig, 1);
  DEFINE_SIG(GuiData_displayClientMessage,
             "40 ? 53 56 57 41 ? 41 ? 48 8D ? ? ? ? ? ? 48 81 EC ? ? ? ? 48 8B "
             "? ? ? ? ? 48 33 ? 48 89 ? ? ? ? ? 45 0F ? ? 49 8B",
             SigType::Sig, 0);
  DEFINE_SIG(InventoryTransaction_addAction,
             "E8 ? ? ? ? 48 81 C3 ? ? ? ? 49 3B ? 75 ? 41 BE", SigType::RefSig,
             1);
  DEFINE_SIG(ItemStack_vTable,
             "48 8D 05 ? ? ? ? 48 89 05 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C6 05 ? "
             "? ? ? ? 48 8D 0D",
             SigType::RefSig, 3);
  DEFINE_SIG(ItemStack_getCustomName,
             "48 89 ? ? ? 57 48 83 EC ? 48 8B ? 48 8B ? E8 ? ? ? ? 84 C0 74 ? "
             "48 8B ? 48 8B ? E8 ? ? ? ? 48 8B",
             SigType::Sig, 0);
  DEFINE_SIG(ItemStack_fromDescriptor,
             "48 89 ? ? ? 48 89 ? ? ? 48 89 ? ? ? 57 48 83 EC ? 0F 57 ? 48 8B",
             SigType::Sig, 0);
  DEFINE_SIG(ItemUseInventoryTransaction_vtable,
             "48 8D ? ? ? ? ? 48 89 ? 8B 46 ? 89 47 ? 0F B6 ? ? 88 47 ? 8B 56",
             SigType::RefSig, 3);
  DEFINE_SIG(ItemUseOnActorInventoryTransaction_vtable,
             "48 8D ? ? ? ? ? 48 89 ? 48 89 ? ? 89 6F ? C7 47 74",
             SigType::RefSig, 3);
  DEFINE_SIG(ItemReleaseInventoryTransaction_vtable,
             "48 8D ? ? ? ? ? 48 89 ? ? 40 88 ? ? ? ? ? 89 AF ? ? ? ? 40 88 ? "
             "? ? ? ? 89 AF ? ? ? ? 0F 57 ? 0F 11 ? ? ? ? ? 48 89 ? ? ? ? ? 48 "
             "C7 87 C8 00 00 00",
             SigType::RefSig, 3); // "gamePlayEmote" -> couple of refs -> this
  DEFINE_SIG(Keyboard_feed,
             "E8 ? ? ? ? E9 ? ? ? ? 41 0F ? ? ? 45 0F ? ? ? 45 0F",
             SigType::RefSig, 1);
  DEFINE_SIG(MainView_instance, "48 8B 05 ? ? ? ? C6 40 ? ? 0F 95 C0",
             SigType::RefSig, 3);
  DEFINE_SIG(MinecraftPackets_createPacket,
             "E8 ? ? ? ? 90 48 83 BD ? ? 00 00 ? 0F 84 ? ? ? ? ff",
             SigType::RefSig, 1);
  DEFINE_SIG(Mob_getJumpControlComponent,
             "E8 ? ? ? ? 48 85 C0 74 ? C6 40 ? ? 48 83 C4 ? 5B",
             SigType::RefSig, 1);
  DEFINE_SIG(Mob_getCurrentSwingDuration, "E8 ? ? ? ? 99 2B C2 D1 F8 8B 8B",
             SigType::RefSig, 1);
  DEFINE_SIG(MouseDevice_feed, "E8 ? ? ? ? 40 88 ? ? ? EB ? 40 84",
             SigType::RefSig, 1);
  DEFINE_SIG(NetworkStackItemDescriptor_ctor,
             "E8 ? ? ? ? 90 48 8B ? 48 8D ? ? ? ? ? E8 ? ? ? ? 4C 8D ? ? ? ? ? "
             "4C 89 ? ? ? ? ? 48 8D",
             SigType::RefSig, 1);
  DEFINE_SIG(PlayerMovement_clearInputStateInlined,
             "75 ? 48 8B ? ? ? ? ? ? 48 8D ? ? ? ? ? ? 48 89 ? ? ? 4D 8B",
             SigType::Sig, 0);
  DEFINE_SIG(PlayerMovement_clearInputStateInlined2,
             "0F 84 ? ? ? ? 48 8B ? ? 48 89 ? ? ? ? ? ? 48 89 ? ? ? ? ? ? 4C "
             "89 ? ? ? ? ? ? 0F 10",
             SigType::Sig, 0);

  DEFINE_SIG(
      RakNet_RakPeer_runUpdateCycle,
      "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? "
      "? ? ? E8 ? ? ? ? 48 2B E0 0F 29 B4 24 ? ? ? ? 0F 29 BC 24 ? ? ? ? 48 8B "
      "05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B EA 48 89 54 24 ? 48 8B D9",
      SigType::Sig, 0);
  DEFINE_SIG(RakNet_RakPeer_sendImmediate,
             "40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 "
             "? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 4C "
             "8B 95 ? ? ? ? 40 32 FF",
             SigType::Sig, 0);
  DEFINE_SIG(ScreenView_setupAndRender,
             "E8 ? ? ? ? 48 8B 44 24 ? 48 8D 4C 24 ? 48 8B 80", SigType::RefSig,
             1);
  DEFINE_SIG(SimulatedPlayer_simulateJump,
             "40 53 48 83 EC ? 48 8B 01 48 8B D9 48 8B 80 ? ? ? ? FF 15 ? ? ? "
             "? 84 C0 0F 84 ? ? ? ? 4C 8B 53",
             SigType::Sig, 0);
  DEFINE_SIG(ItemInHandRenderer_render_bytepatch,
             "F3 0F ? ? ? ? ? ? 48 8B ? F3 41 ? ? ? 0F 57", SigType::Sig, 0);
  DEFINE_SIG(SneakMovementSystem_tickSneakMovementSystem,
             "32 C0 41 88 41 ? 84 C0", SigType::Sig, 0);
  DEFINE_SIG(ConnectionRequest_create,
             "40 ? 53 56 57 41 ? 41 ? 41 ? 41 ? 48 8D ? ? ? ? ? ? 48 81 EC ? ? "
             "? ? 0F 29 ? ? ? ? ? ? 48 8B ? ? ? ? ? 48 33 ? 48 89 ? ? ? ? ? 49 "
             "8B ? 48 89 ? ? ? ? ? 48 89 ? ? 48 89",
             SigType::Sig, 0);
  DEFINE_SIG(
      CameraDirectLookSystemUtil_handleLookInput,
      "48 89 ? ? ? 57 48 83 EC ? F3 41 ? ? ? 49 8B ? F3 41 ? ? ? ? F3 0F",
      SigType::Sig, 0);
  DEFINE_SIG(ItemRenderer_render,
             "48 8B ? 48 89 ? ? 55 56 57 41 ? 41 ? 41 ? 41 ? 48 81 EC ? ? ? ? "
             "0F 29 ? ? 0F 29 ? ? 44 0F ? ? ? 44 0F ? ? ? 49 8B",
             SigType::Sig, 0);
  DEFINE_SIG(
      ItemPositionConst,
      "F3 0F ? ? ? ? ? ? F3 0F ? ? F3 0F ? ? F3 0F ? ? ? ? ? ? F3 0F ? ? 0F B7",
      SigType::Sig, 0);
  /*DEFINE_SIG(glm_rotate, "40 53 48 83 EC ? F3 0F 59 0D ? ? ? ? 4C 8D 4C 24",
  SigType::Sig, 0); DEFINE_SIG(glm_rotateRef, "E8 ? ? ? ? 0F 28 ? ? ? ? ? 48 8B
  ? C6 40 38", SigType::Sig, 0); DEFINE_SIG(glm_translateRef, "E8 ? ? ? ? E9 ? ?
  ? ? 40 84 ? 0F 84 ? ? ? ? 83 FF", SigType::Sig, 0);
  DEFINE_SIG(glm_translateRef2, "E8 ? ? ? ? C6 46 ? ? F3 0F 11 74 24 ? F3 0F 10
  1D", SigType::Sig, 0);*/
  DEFINE_SIG(BlockSource_fireBlockChanged, "4C 8B ? 45 89 ? ? 49 89 ? ? 53",
             SigType::Sig, 0);
  DEFINE_SIG(ActorAnimationControllerPlayer_applyToPose,
             "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 57 41 54 41 56 48 "
             "81 EC ? ? ? ? 4C 63 B4 24 ? ? ? ? 45 33 E4 4C 89 A2 ? ? ? ?",
             SigType::Sig, 0);
  DEFINE_SIG(JSON_parse,
             "E8 ? ? ? ? 0F B6 D8 48 8D 8D ? ? ? ? E8 ? ? ? ? 90 48 8D 8D ? ? "
             "? ? E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? C6 44 24",
             SigType::RefSig, 1);
  DEFINE_SIG(Actor_getStatusFlag, "E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? F3 0F 10 BF",
             SigType::RefSig,
             1); // bro the amount of references for this went from 43 to 4.
  DEFINE_SIG(Level_getRuntimeActorList,
             "48 89 ? ? ? 55 56 57 48 83 EC ? 48 8B ? 48 89 ? ? ? 33 D2",
             SigType::Sig, 0);
  DEFINE_SIG(ConcreteBlockLegacy_getCollisionShapeForCamera,
             "40 ? 48 83 EC ? 48 8B ? ? ? ? ? 48 33 ? 48 89 ? ? ? 4C 8B ? 4D "
             "8B ? 49 8B",
             SigType::Sig, 0);
  DEFINE_SIG(ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil,
             "FF 15 ? ? ? ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 32 DB",
             SigType::Sig, 0);
  DEFINE_SIG(
      WaterBlockLegacy_getCollisionShapeForCamera,
      "0F 10 ? ? ? ? ? 48 8B ? F2 0F ? ? ? ? ? ? 0F 11 ? F2 0F ? ? ? C3 CC",
      SigType::Sig, 0);
  DEFINE_SIG(mce_framebuilder_RenderItemInHandDescription_ctor,
             "48 89 ? ? ? 48 89 ? ? ? 55 56 57 41 ? 41 ? 41 ? 41 ? 48 83 EC ? "
             "4D 8B ? 4D 8B ? 4C 8B ? 48 8B ? 45 33",
             SigType::Sig, 0);

  DEFINE_SIG(ResourcePackManager_composeFullStackBp,
             "0F 84 ? ? ? ? 48 8B ? E8 ? ? ? ? 84 C0 74 ? 0F 57", SigType::Sig,
             0);
  DEFINE_SIG(ClientInstance_isPreGame,
             "48 83 EC ? 48 8B ? 48 8B ? ? ? ? ? FF 15 ? ? ? ? 48 85 ? 0F 94",
             SigType::Sig, 0);
  DEFINE_SIG(
      tickEntity_ItemUseSlowdownModifierComponent,
      "48 89 ? ? ? 48 89 ? ? ? 57 48 83 EC ? 49 8B ? 4D 85", SigType::Sig,
      0); // ?tickEntity@?$Impl@U?$type_list@AEBUItemInUseComponent@@V?$EntityModifier@UItemUseSlowdownModifierComponent@@@@@entt@@U?$type_list@U?$type_list@U?$Include@V?$FlagComponent@UActorMovementTickNeededFlag@@@@UPlayerInputRequestComponent@@@@U?$Exclude@UPassengerComponent@@@@@entt@@AEBVStrictEntityContext@@AEBUItemInUseComponent@@V?$EntityModifier@UItemUseSlowdownModifierComponent@@@@@2@U?$type_list@V?$EntityModifier@UItemUseSlowdownModifierComponent@@@@@2@@?$CandidateAdapter@$MP6AXU?$type_list@U?$Include@V?$FlagComponent@UActorMovementTickNeededFlag@@@@UPlayerInputRequestComponent@@@@U?$Exclude@UPassengerComponent@@@@@entt@@AEBVStrictEntityContext@@AEBUItemInUseComponent@@V?$EntityModifier@UItemUseSlowdownModifierComponent@@@@@Z1?doItemUseSlowdownSystem@ItemUseSlowdownSystemImpl@@YAX0123@Z@details@@SAXAEBVStrictEntityContext@@AEBUItemInUseComponent@@V?$EntityModifier@UItemUseSlowdownModifierComponent@@@@@Z
  DEFINE_SIG(checkBlocks, "48 8D 05 ? ? ? ? 4D 8B CE 48 89 44 24 ? 4C 8D 44 24",
             SigType::Sig, 0);
  DEFINE_SIG(JSON_toStyledString,
             "48 89 5C 24 ? 56 57 41 56 48 83 EC ? 33 C0 0F 57 C0 ? ? ? 48 89 "
             "41 ? 4C 8B F2 48 89 41 ? 48 8B F9",
             SigType::RefSig, 1);

  // TODO: Identify proper function names for these and refactor them
  // accordingly

  DEFINE_SIG(Unknown_renderBlockOverlay,
             "40 ? 53 56 57 41 ? 41 ? 48 8D ? ? ? ? ? ? 48 81 EC ? ? ? ? 49 8B "
             "? 49 8B ? 4C 8B ? 48 8B ? 48 8B",
             SigType::Sig, 0);
  DEFINE_SIG(FastEat, "FF C8 89 87 ? ? ? ? 83 F8", SigType::Sig, 0);
  DEFINE_SIG(Unknown_renderNametag,
             "48 8B ? 55 53 56 57 41 ? 41 ? 41 ? 48 8D ? ? 48 81 EC ? ? ? ? 0F "
             "29 ? ? 0F 29 ? ? 48 8B ? ? ? ? ? 48 33 ? 48 89 ? ? 4C 89",
             SigType::Sig, 0);
  DEFINE_SIG(Reach, "74 ? F3 44 ? ? ? ? ? ? ? EB ? F3 0F", SigType::Sig, 0);
  DEFINE_SIG(BlockReach, "F3 0F ? ? ? ? ? ? 48 8B ? ? ? 48 83 C4 ? 5F C3 83 C0",
             SigType::Sig, 0);
  DEFINE_SIG(GetSpeedInAirWithSprint, "41 C7 40 ? ? ? ? ? F6 02", SigType::Sig,
             0);
  DEFINE_SIG(ConnectionRequest_create_DeviceModel, "48 8B 11 48 83 C2 ? EB",
             SigType::Sig, 0);
  DEFINE_SIG(ConnectionRequest_create_DeviceOS,
             "BA ? ? ? ? 0F 44 ? C3 CC CC CC CC CC CC CC CC CC", SigType::Sig,
             0);
  DEFINE_SIG(
      ConnectionRequest_create_DefaultInputMode,
      "C6 44 24 58 ? 48 63 ? 48 89 ? ? ? 48 8D ? ? ? ? ? 48 8D ? ? ? ? ? E8 ? "
      "? ? ? 48 8B ? 33 D2 48 8D ? ? ? E8 ? ? ? ? 0F B6 ? ? 0F B6 ? ? ? 88 44 "
      "? ? 48 8B ? 48 8B ? ? ? 48 89 ? ? ? C6 43 08 ? 88 54 ? ? 48 89 ? 48 89 "
      "? ? ? 48 8D ? ? ? E8 ? ? ? ? 90 48 8D ? ? ? E8 ? ? ? ? C6 44 24 58",
      SigType::Sig, 0); // this shit is so fat
  DEFINE_SIG(
      ConnectionRequest_create_CurrentInputMode,
      "48 63 ? ? ? ? ? 48 89 ? ? ? 48 8D ? ? ? ? ? 48 8D ? ? ? ? ? E8 ? ? ? ? "
      "48 8B ? 33 D2 48 8D ? ? ? E8 ? ? ? ? 0F B6 ? ? 0F B6 ? ? ? 88 44 ? ? 48 "
      "8B ? 48 8B ? ? ? 48 89 ? ? ? C6 43 08 ? 88 54 ? ? 48 89 ? 48 89 ? ? ? "
      "48 8D ? ? ? E8 ? ? ? ? 90 48 8D ? ? ? E8 ? ? ? ? C6 44 24 58 ? 48 63 ? "
      "? ? ? ? 48 89 ? ? ? 48 8D ? ? ? ? ? 48 8D ? ? ? ? ? E8 ? ? ? ? 48 8B ? "
      "33 D2 48 8D ? ? ? E8 ? ? ? ? 0F B6 ? ? 0F B6 ? ? ? 88 44 ? ? 48 8B ? 48 "
      "8B ? ? ? 48 89 ? ? ? C6 43 08 ? 88 54 ? ? 48 89 ? 48 89 ? ? ? 48 8D ? ? "
      "? E8 ? ? ? ? 90 48 8D ? ? ? E8 ? ? ? ? C6 44 24 58",
      SigType::Sig, 0); // this shit is so fat
  DEFINE_SIG(InputModeBypass,
             "8b d7 48 8b ce 48 8b 80 ? ? ? ? ff 15 ? ? ? ? 49 8b 07",
             SigType::Sig, 0);
  DEFINE_SIG(InputModeBypassFix,
             "49 8b 07 8b d7 49 8b cf 48 8b 80 ? ? ? ? ff 15 ? ? ? ? 49 8b 07",
             SigType::Sig,
             0); // fixes gui bugs, withot it ur inv will works like on mobile
  DEFINE_SIG(TapSwingAnim,
             "F3 44 ? ? ? ? ? ? ? 4C 8D ? ? ? 48 8B ? 48 C7 44 24 30 ? ? ? ? "
             "c7 44 24 ? ? ? ? ? 8b 08",
             SigType::RefSig, 5);
  DEFINE_SIG(Unknown_updatePlayerFromCamera,
             "4C 8B ? 49 89 ? ? 49 89 ? ? 55 41 ? 41 ? 49 8D", SigType::Sig, 0);

  DEFINE_SIG(FluxSwing,
             "E8 ? ? ? ? 48 8B ? F3 0F ? ? ? ? ? ? F3 0F ? ? ? ? ? ? F3 0F ? ? "
             "? ? ? ? C6 40 38 ? 48 8B ? EB",
             SigType::Sig, 0);
  DEFINE_SIG(BobHurt,
             "48 89 5C 24 ? 57 48 81 EC ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 81 C1",
             SigType::Sig, 0);
  DEFINE_SIG(CameraComponent_applyRotation,
             "66 0F ? ? 0F 5B ? 0F 2F ? 76 ? F3 0F ? ? F3 0F", SigType::Sig,
             0); // Guessed func name
  DEFINE_SIG(
      FireRender,
      "48 8B ? 55 53 56 57 41 ? 41 ? 41 ? 41 ? 48 8D ? ? ? ? ? 48 81 EC ? ? ? "
      "? 0F 29 ? ? 0F 29 ? ? 44 0F ? ? ? 44 0F ? ? ? ? ? ? 44 0F ? ? ? ? ? ? "
      "44 0F ? ? ? ? ? ? 48 8B ? ? ? ? ? 48 33 ? 48 89 ? ? ? ? ? 4D 8B ? 4D 8B",
      SigType::Sig, 0);
  DEFINE_SIG(Actor_canSee,
             "E8 ? ? ? ? 84 C0 74 ? 48 8B ? ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? "
             "FF 15 ? ? ? ? E9",
             SigType::RefSig, 1);

  DEFINE_SIG(ItemInHandRenderer_renderItem_bytepatch,
             "F3 0F ? ? ? ? ? ? 0F 57 ? F3 0F ? ? ? ? ? ? F3 0F ? ? 0F 2F ? 73 "
             "? F3 41",
             SigType::Sig, 0);
  DEFINE_SIG(ItemInHandRenderer_renderItem_bytepatch2,
             "8B 52 ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 48 8B ? EB ? 48 8D ? ? ? "
             "? ? 48 8B ? ? ? ? ? 48 8B",
             SigType::Sig, 0);

  // HurtColor — RenderController::getOverlayColor
  // Hooks the function that returns the overlay color when an entity takes damage
  // Signature from Flarial init2140 for 1.21.4X
  DEFINE_SIG(HurtColor,
             "E8 ? ? ? ? 0F 28 05 ? ? ? ? 0F 11 85 68 02 00 00",
             SigType::RefSig, 1);

  // ═══════════════════════════════════════════════════════════════
  // Flarial SigInit::init2140() — COMMENTED OUT to avoid slow inject
  // Uncomment individual sigs ONLY when you create a hook/module for them
  // ═══════════════════════════════════════════════════════════════

  // --- Rendering / Visual ---
  // DEFINE_SIG(Font_drawTransformed, "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 57 41 54 41 55 41 56 41 57 48 81 EC B0 01", SigType::Sig, 0);
  // DEFINE_SIG(BaseActorRenderer_renderText, "E8 ? ? ? ? 48 83 C3 70 48 3B DE 75 D3", SigType::RefSig, 1);
  // DEFINE_SIG(ThirdPersonNametag, "0F 84 B5 05 00 00 49 8B 07 49 8B CF 48 8B 80 00 01 00 00", SigType::Sig, 0);
  // DEFINE_SIG(ScreenRenderer_blit, "48 89 5C 24 08 57 48 83 EC 60 0F 57", SigType::Sig, 0);
  // DEFINE_SIG(mce_RenderMaterialGroup_ui, "48 8B 05 ? ? ? ? 48 8D 55 90 48 8D 0D ? ? ? ? 48 8B 40 08 FF 15 ? ? ? ? 48 8B D8", SigType::RefSig, 3);
  // DEFINE_SIG(blockHighlightColor, "? ? ? ? 0F 11 85 78 02 00 00 0F 11 00", SigType::Sig, 0);
  // DEFINE_SIG(mce_Color_BLACK, "? ? ? ? 0F 11 00 C6 40 10 ? 0F 57 C9", SigType::Sig, 0);
  // DEFINE_SIG(LevelRendererPlayer_renderOutlineSelection, "E8 ? ? ? ? 90 48 8D 8D ? ? ? ? E8 ? ? ? ? 48 8B 86 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 80 38 ? 0F 84 ? ? ? ? 48 8B 86 ? ? ? ? 48 85 C0 74 ? F0 FF 40 ? 48 8B 86", SigType::RefSig, 1);

  // --- Tessellator / Mesh ---
  // DEFINE_SIG(Tessellator_begin, "40 53 55 48 83 EC 28 80 B9", SigType::Sig, 0);
  // DEFINE_SIG(MeshHelpers_renderMeshImmediately, "E8 ? ? ? ? 41 C6 44 24 38 01 F3 0F 10 1D", SigType::RefSig, 1);
  // DEFINE_SIG(MeshHelpers_renderMeshImmediately2, "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 98 FC FF FF 48 81 EC 68 04 00 00 49", SigType::Sig, 0);

  // --- GLM Math ---
  // DEFINE_SIG(glm_rotateRef, "E8 ? ? ? ? 48 8B D0 8B 08 89 0B 8B 48 04 89 4B 04 8B 48 08 89 4B 08 8B 48 0C 89 4B 0C 8B 48 10 89 4B 10 8B 48 14 89 4B 14 8B 40 18 89 43 18 8B 42 1C 89 43 1C 8B 42 20 89 43 20 8B 42 24 89 43 24 8B 42 28 89 43 28 8B 42 2C 89 43 2C 8B 42 30 89 43 30 8B 42 34 89 43 34 8B 42 38 89 43 38 8B 42 3C 89 43 3C 0F B6 47 22", SigType::RefSig, 1);
  // DEFINE_SIG(glm_translateRef, "E8 ? ? ? ? 48 8D 15 ? ? ? ? 0F 28 35", SigType::RefSig, 1);

  // --- Camera ---
  // DEFINE_SIG(CameraYaw, "F3 0F 11 00 F3 0F 11 70 04", SigType::Sig, 0);
  // DEFINE_SIG(CameraYaw2, "F3 0F 11 10 48 8B 8F 30 02 00 00", SigType::Sig, 0);
  // DEFINE_SIG(UpdatePlayer, "48 89 5C 24 08 57 48 83 EC 70 0F 29 74 24 60 49", SigType::Sig, 0);

  // --- Fog / Dimension ---
  // DEFINE_SIG(Dimension_getBrightnessDependentFogColor, "41 0F 10 00 48 8B C2 0F", SigType::Sig, 0);
  // DEFINE_SIG(BlockSource_getBiome, "48 89 5C 24 18 57 48 83 EC 60 48 8B DA 48 8B F9 48 8B", SigType::Sig, 0);

  // --- Items ---
  // DEFINE_SIG(ItemStack_getDamageValue, "40 53 48 83 EC 30 48 8B 51 08 33", SigType::Sig, 0);
  // DEFINE_SIG(ItemStack_isEnchanted, "48 83 EC 38 48 8B 49 10 48 85 C9 74 60", SigType::Sig, 0);

  // --- Entity ---
  // DEFINE_SIG(Actor_vtable, "48 8D 05 ? ? ? ? 48 89 01 ? ? ? ? 00 44 8D 4A 04 44 8D 42 02 66 C7 44 24 20 37 00 E8 ? ? ? ? 48 8B 8F 80 11 00 00", SigType::RefSig, 3);
  // DEFINE_SIG(HitResult_getEntity, "40 55 57 41 54 41 56 41 57 48 83", SigType::Sig, 0);

  // --- Network ---
  // DEFINE_SIG(RaknetTick, "4C 8B DC 49 89 5B 10 49 89 6B 18 56 57 41 56 48 81 EC 10", SigType::Sig, 0);

  // --- Options ---
  // DEFINE_SIG(Options_getSensitivity, "48 83 EC 18 48 8B 41 08 4C 8B C9", SigType::Sig, 0);

  // --- UI ---
  // DEFINE_SIG(UIControl_getPosition, "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 60 0F 29 74 24 50 0F 29 7C 24 40 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 30", SigType::Sig, 0);
  // DEFINE_SIG(ClientInstance_updateScreenSizeVariables, "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 28 FF FF FF 48 81 EC A0 01 00 00 0F 29 70 B8 0F 29 78 A8 44 0F 29 40 98 44 0F 29 48 88 44 0F 29 90 78 FF FF FF 44 0F 29 98 68 FF FF FF 44 0F 29 A0 58 FF FF FF 48", SigType::Sig, 0);
  // DEFINE_SIG(GeneralSettingsScreenController_ctor, "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 30 FB FF FF 48 81 EC D0 05 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 C0 04 00 00 45", SigType::Sig, 0);
  // DEFINE_SIG(SettingsScreenOnExit, "40 57 48 83 EC 40 48 8B 79", SigType::Sig, 0);
  // DEFINE_SIG(SettingsScreenOnExit_Patch, "74 ? 48 8b 8f ? ? ? ? e8 ? ? ? ? 33 c0", SigType::Sig, 0);
  // DEFINE_SIG(ContainerScreenController_onContainerSlotHovered, "48 89 ? ? ? 57 41 ? 41 ? 48 83 EC ? 45 8B ? 48 8B ? 48 8B ? 44 39", SigType::Sig, 0);
  // DEFINE_SIG(MinecraftGame_onResumeWaitReloadActors, "48 89 5C 24 10 48 89 74 24 18 55 57 41 54 41 56 41 57 48 8D 6C 24 C9 48 81 EC D0 00 00 00 4C 8B F9 48", SigType::Sig, 0);

  // --- BoneTransform (for custom animations) ---
  // DEFINE_SIG(BoneOrientation_computeBoneTransform, "48 8B C4 53 57 48 81 EC 38 01 00 00 80 B9 DE 00 00 00 00", SigType::Sig, 0);

  static void initialize();
  static void deinitialize();
};
