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
  DEFINE_SIG(HudCursorRenderer_render,
             "48 8B C4 48 89 58 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ? 0F 29 78 ? 44 0F 29 40 ? 44 0F 29 48 ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 89 8D",
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
  DEFINE_SIG(ComplexInventoryTransaction_vtable,
             "48 8D 05 ?? ?? ?? ?? 48 89 07 C7 47 08 03 00 00 00 48 8D 5F 10",
             SigType::RefSig, 3);
  DEFINE_SIG(GameMode_getDestroyRate, "E8 ? ? ? ? 0F 28 ? 49 8B ? ? E8",
             SigType::RefSig, 1);
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
  DEFINE_SIG(NetworkStackItemDescriptor_ctor,
             "E8 ? ? ? ? 90 48 8B ? 48 8D ? ? ? ? ? E8 ? ? ? ? 4C 8D ? ? ? ? ? "
             "4C 89 ? ? ? ? ? 48 8D",
             SigType::RefSig, 1);

  DEFINE_SIG(ScreenView_setupAndRender,
             "E8 ? ? ? ? 48 8B 44 24 ? 48 8D 4C 24 ? 48 8B 80", SigType::RefSig,
             1);
  DEFINE_SIG(SimulatedPlayer_simulateJump,
             "40 53 48 83 EC ? 48 8B 01 48 8B D9 48 8B 80 ? ? ? ? FF 15 ? ? ? "
             "? 84 C0 0F 84 ? ? ? ? 4C 8B 53",
             SigType::Sig, 0);
  DEFINE_SIG(ConnectionRequest_create,
             "40 ? 53 56 57 41 ? 41 ? 41 ? 41 ? 48 8D ? ? ? ? ? ? 48 81 EC ? ? "
             "? ? 0F 29 ? ? ? ? ? ? 48 8B ? ? ? ? ? 48 33 ? 48 89 ? ? ? ? ? 49 "
             "8B ? 48 89 ? ? ? ? ? 48 89 ? ? 48 89",
             SigType::Sig, 0);
  DEFINE_SIG(JSON_parse,
             "E8 ? ? ? ? 0F B6 D8 48 8D 8D ? ? ? ? E8 ? ? ? ? 90 48 8D 8D ? ? "
             "? ? E8 ? ? ? ? 84 DB 0F 84 ? ? ? ? C6 44 24",
             SigType::RefSig, 1);
  DEFINE_SIG(Level_getRuntimeActorList,
             "48 89 ? ? ? 55 56 57 48 83 EC ? 48 8B ? 48 89 ? ? ? 33 D2",
             SigType::Sig, 0);

  DEFINE_SIG(checkBlocks, "48 8D 05 ? ? ? ? 4D 8B CE 48 89 44 24 ? 4C 8D 44 24",
             SigType::Sig, 0);
  DEFINE_SIG(JSON_toStyledString,
             "48 89 5C 24 ? 56 57 41 56 48 83 EC ? 33 C0 0F 57 C0 ? ? ? 48 89 "
             "41 ? 4C 8B F2 48 89 41 ? 48 8B F9",
             SigType::RefSig, 1);

  // TODO: Identify proper function names for these and refactor them
  // accordingly

  DEFINE_SIG(Reach, "74 ? F3 44 ? ? ? ? ? ? ? EB ? F3 0F", SigType::Sig, 0);
  DEFINE_SIG(BlockReach, "F3 0F ? ? ? ? ? ? 48 8B ? ? ? 48 83 C4 ? 5F C3 83 C0",
             SigType::Sig, 0);
  DEFINE_SIG(ConnectionRequest_create_DeviceModel, "48 8B 11 48 83 C2 ? EB",
             SigType::Sig, 0);

  DEFINE_SIG(Actor_canSee,
             "E8 ? ? ? ? 84 C0 74 ? 48 8B ? ? 48 8B ? 48 8B ? 48 8B ? ? ? ? ? "
             "FF 15 ? ? ? ? E9",
             SigType::RefSig, 1);

  DEFINE_SIG(ItemInHandRenderer_renderItem_bytepatch2,
             "8B 52 ? 48 8B ? 48 8B ? ? FF 15 ? ? ? ? 48 8B ? EB ? 48 8D ? ? ? "
             "? ? 48 8B ? ? ? ? ? 48 8B",
             SigType::Sig, 0);

  static void initialize();
  static void deinitialize();
};
