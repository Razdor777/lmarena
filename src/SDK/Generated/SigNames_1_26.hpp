// СГЕНЕРЕНО tools/sig_map.py — не править руками
//
// Сопоставление имён из SigManager с именами символов 1.26.
// SymDB ищет по `symbol`; если пусто — имя в 1.26 неизвестно.

#pragma once

#include <cstddef>
#include <string>

namespace SigNames {

struct Entry {
    const char* sig;      // имя из SigManager
    const char* symbol;   // имя символа в 1.26, "" если неизвестно
    const char* status;   // FOUND / RENAMED / MOVED / GONE / NO_CLASS / NOT_IN_CLASS
};

inline constexpr Entry kTable[] = {
    { "Actor_setPosition", "Actor::_setPos", "RENAMED" },
    { "HudCursorRenderer_render", "HudCursorRenderer::render", "FOUND" },
    { "Actor_getNameTag", "Actor::getNameTag", "FOUND" },
    { "Actor_setNameTag", "Actor::setNameTag", "FOUND" },
    { "ActorRenderDispatcher_render", "ActorRenderDispatcher::render", "FOUND" },
    { "ClientInstance_mBgfx", "", "GONE" },
    { "ClientInstance_grabMouse", "ClientInstance::grabMouse", "FOUND" },
    { "ClientInstance_releaseMouse", "ClientInstance::releaseMouse", "FOUND" },
    { "ContainerScreenController_tick", "ContainerScreenController::tick", "FOUND" },
    { "ContainerScreenController_tryBeginTransition", "", "GONE" },
    { "ContainerScreenController_handleAutoPlace", "ContainerScreenController::_handleAutoPlace", "FOUND" },
    { "ContainerValidation_handleTransfer_jnz", "ScreenData::handleTransfer_jnz", "MOVED" },
    { "ComplexInventoryTransaction_vtable", "ComplexInventoryTransaction::ComplexInventoryTransaction", "FOUND" },
    { "isCriticalHit", "", "NO_CLASS" },
    { "EnchantUtils_getEnchantLevel", "EnchantUtils::getEnchantLevel", "FOUND" },
    { "GameMode_getDestroyRate", "GameMode::getDestroyRate", "FOUND" },
    { "ContainerValidation_handlePlace_jnz", "", "NOT_IN_CLASS" },
    { "ContainerValidation_handlePlace_test", "", "NOT_IN_CLASS" },
    { "HoverTextRenderer_render", "HoverTextRenderer::render", "FOUND" },
    { "GameMode_baseUseItem", "GameMode::baseUseItem", "FOUND" },
    { "GuiData_displayClientMessage", "GuiData::displayClientMessage", "FOUND" },
    { "InventoryTransaction_addAction", "InventoryTransaction::addAction", "FOUND" },
    { "ItemStack_vTable", "", "NOT_IN_CLASS" },
    { "ItemStack_getCustomName", "ComparisonOptions::getCustomName", "MOVED" },
    { "ItemStack_fromDescriptor", "ItemStack::fromDescriptor", "FOUND" },
    { "ItemUseInventoryTransaction_vtable", "", "NOT_IN_CLASS" },
    { "ItemUseOnActorInventoryTransaction_vtable", "", "NOT_IN_CLASS" },
    { "ItemReleaseInventoryTransaction_vtable", "", "NOT_IN_CLASS" },
    { "Keyboard_feed", "", "GONE" },
    { "MainView_instance", "", "GONE" },
    { "MinecraftPackets_createPacket", "MinecraftPackets::createPacket", "FOUND" },
    { "Mob_getJumpControlComponent", "Mob::getJumpControlComponent", "MOVED" },
    { "Mob_getCurrentSwingDuration", "Mob::getModifiedSwingDuration", "RENAMED" },
    { "MouseDevice_feed", "", "GONE" },
    { "NetworkStackItemDescriptor_ctor", "", "NO_CLASS" },
    { "PlayerMovement_clearInputStateInlined", "", "NOT_IN_CLASS" },
    { "PlayerMovement_clearInputStateInlined2", "", "NOT_IN_CLASS" },
    { "RakNet_RakPeer_runUpdateCycle", "", "NOT_IN_CLASS" },
    { "RakNet_RakPeer_sendImmediate", "", "GONE" },
    { "ScreenView_setupAndRender", "", "GONE" },
    { "SimulatedPlayer_simulateJump", "SimulatedPlayer::simulateJump", "FOUND" },
    { "ItemInHandRenderer_render_bytepatch", "Page::render_bytepatch", "MOVED" },
    { "SneakMovementSystem_tickSneakMovementSystem", "", "NOT_IN_CLASS" },
    { "ConnectionRequest_create", "", "GONE" },
    { "CameraDirectLookSystemUtil_handleLookInput", "", "GONE" },
    { "ItemRenderer_render", "ItemRenderer::render", "FOUND" },
    { "ItemInHandRenderer_renderItem", "ItemInHandRenderer::renderItem", "FOUND" },
    { "ItemEntityRenderer_render", "", "NO_CLASS" },
    { "ColorTint_set", "", "NO_CLASS" },
    { "ItemRenderer_renderGuiItem", "", "NOT_IN_CLASS" },
    { "ItemPositionConst", "", "NO_CLASS" },
    { "glm_rotate", "", "GONE" },
    { "glm_rotateRef", "", "GONE" },
    { "glm_translateRef", "", "GONE" },
    { "glm_translateRef2", "", "GONE" },
    { "BlockSource_fireBlockChanged", "BlockSource::fireBlockChanged", "FOUND" },
    { "ActorAnimationControllerPlayer_applyToPose", "ActorAnimationControllerPlayer::applyToPose", "FOUND" },
    { "JSON_parse", "", "GONE" },
    { "Actor_getStatusFlag", "Actor::getStatusFlag", "FOUND" },
    { "Level_getRuntimeActorList", "Level::getRuntimeActorList", "FOUND" },
    { "ConcreteBlockLegacy_getCollisionShapeForCamera", "", "GONE" },
    { "ClientInputCallbacks_handleBuildAction_onAttack_setNoBlockBreakUntil", "", "NOT_IN_CLASS" },
    { "WaterBlockLegacy_getCollisionShapeForCamera", "", "NO_CLASS" },
    { "mce_framebuilder_RenderItemInHandDescription_ctor", "", "NO_CLASS" },
    { "ResourcePackManager_composeFullStackBp", "ResourcePackManager::composeFullStack", "RENAMED" },
    { "ClientInstance_isPreGame", "ClientInstance::isPreGame", "FOUND" },
    { "tickEntity_ItemUseSlowdownModifierComponent", "", "NO_CLASS" },
    { "checkBlocks", "", "NO_CLASS" },
    { "JSON_toStyledString", "T::toStyledString", "MOVED" },
    { "Unknown_renderBlockOverlay", "", "NO_CLASS" },
    { "FastEat", "", "NO_CLASS" },
    { "Unknown_renderNametag", "", "NO_CLASS" },
    { "Reach", "", "NO_CLASS" },
    { "BlockReach", "", "NO_CLASS" },
    { "GetSpeedInAirWithSprint", "", "NO_CLASS" },
    { "ConnectionRequest_create_DeviceModel", "", "NOT_IN_CLASS" },
    { "ConnectionRequest_create_DeviceOS", "", "NOT_IN_CLASS" },
    { "ConnectionRequest_create_DefaultInputMode", "", "NOT_IN_CLASS" },
    { "ConnectionRequest_create_CurrentInputMode", "", "NOT_IN_CLASS" },
    { "InputModeBypass", "", "NO_CLASS" },
    { "InputModeBypassFix", "", "NO_CLASS" },
    { "TapSwingAnim", "", "NO_CLASS" },
    { "Unknown_updatePlayerFromCamera", "", "NO_CLASS" },
    { "FluxSwing", "", "NO_CLASS" },
    { "BobHurt", "", "NO_CLASS" },
    { "CameraComponent_applyRotation", "", "NOT_IN_CLASS" },
    { "FireRender", "", "NO_CLASS" },
    { "Actor_canSee", "Actor::canSee", "FOUND" },
    { "ItemInHandRenderer_renderItem_bytepatch", "ItemInHandRenderer::renderItem", "FOUND" },
    { "ItemInHandRenderer_renderItem_bytepatch2", "", "NOT_IN_CLASS" },
    { "HurtColor", "", "NO_CLASS" },
    { "Font_drawTransformed", "Font::drawTransformed", "FOUND" },
    { "BaseActorRenderer_renderText", "SignRenderer::renderText", "MOVED" },
    { "ThirdPersonNametag", "", "NO_CLASS" },
    { "ScreenRenderer_blit", "ScreenRenderer::blit", "FOUND" },
    { "mce_RenderMaterialGroup_ui", "", "NO_CLASS" },
    { "blockHighlightColor", "", "NO_CLASS" },
    { "mce_Color_BLACK", "", "NO_CLASS" },
    { "LevelRendererPlayer_renderOutlineSelection", "LevelRendererPlayer::_renderOutlineSelection", "FOUND" },
    { "Tessellator_begin", "Tessellator::begin", "FOUND" },
    { "MeshHelpers_renderMeshImmediately", "MeshHelpers::renderMeshImmediately", "FOUND" },
    { "MeshHelpers_renderMeshImmediately2", "", "NOT_IN_CLASS" },
    { "glm_rotateRef", "", "GONE" },
    { "glm_translateRef", "", "GONE" },
    { "CameraYaw", "", "NO_CLASS" },
    { "CameraYaw2", "", "NO_CLASS" },
    { "UpdatePlayer", "", "NO_CLASS" },
    { "Dimension_getBrightnessDependentFogColor", "Dimension::getBrightnessDependentFogColor", "FOUND" },
    { "BlockSource_getBiome", "BlockSource::getBiome", "FOUND" },
    { "ItemStack_getDamageValue", "OffhandAllowed::getDamageValue", "MOVED" },
    { "ItemStack_isEnchanted", "Vec3::isEnchanted", "MOVED" },
    { "Actor_vtable", "", "NOT_IN_CLASS" },
    { "HitResult_getEntity", "HitResult::getEntity", "FOUND" },
    { "RaknetTick", "", "NO_CLASS" },
    { "Options_getSensitivity", "", "NO_CLASS" },
    { "UIControl_getPosition", "WriteBufferInfo::getPosition", "MOVED" },
    { "ClientInstance_updateScreenSizeVariables", "ClientInstance::_updateScreenSizeVariables", "FOUND" },
    { "GeneralSettingsScreenController_ctor", "GeneralSettingsScreenController::ctor", "FOUND" },
    { "SettingsScreenOnExit", "", "NO_CLASS" },
    { "SettingsScreenOnExit_Patch", "", "NO_CLASS" },
    { "ContainerScreenController_onContainerSlotHovered", "ContainerScreenController::_onContainerSlotHovered", "FOUND" },
    { "MinecraftGame_onResumeWaitReloadActors", "MinecraftGame::_onResumeWaitReloadActors", "FOUND" },
    { "BoneOrientation_computeBoneTransform", "", "NOT_IN_CLASS" },
};

inline constexpr size_t kCount = sizeof(kTable) / sizeof(kTable[0]);

// Имя символа 1.26 для записи SigManager; пустая строка, если имя неизвестно.
inline const char* symbolOf(const std::string& sigName) {
    for (size_t i = 0; i < kCount; ++i)
        if (sigName == kTable[i].sig) return kTable[i].symbol;
    return "";
}

}  // namespace SigNames
