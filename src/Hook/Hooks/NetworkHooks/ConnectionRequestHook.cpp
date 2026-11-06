//
// Created by vastrakai on 7/3/2024.
//

#include "ConnectionRequestHook.hpp"
#include <Features/Events/ConnectionRequestEvent.hpp>
#include <Features/Modules/Misc/EditionFaker.hpp>
#include <SDK/Minecraft/Actor/SerializedSkin.hpp>

std::unique_ptr<Detour> ConnectionRequestHook::mDetour;

struct ConnectionSkinInfo {
    std::string* skinId;
    SerializedSkin* skin;
    char _pad[0x10];
    __int64 other;
    void** holder;
};

void* ConnectionRequestHook::createRequestDetourFunc(
    __int64 _this,
    __int64/*Bedrock::NonOwnerPointer<::AppPlatform>*/ appPlatform,
    class ConnectionAuthInfo* authInfo,
    std::string* selfSignedId,
    std::string* serverAddress,
    unsigned __int64 clientRandomId,
    class ConnectionSkinInfo const* skinInfo,
    std::string* deviceId,
    int /*InputMode*/ currentInputMode,
    int guiScale,
    std::string const& languageCode,
    bool clientIsEditorCapable,
    int /*EditorConnectionJoinIntent*/ clientEditorConnectionIntent,
    bool isEduMode,
    void* /*std::optional<class WebToken>*/ eduTokenChain,
    std::string eduSessionToken,
    std::string eduJoinerToHostNonce,
    unsigned char /*edu::Role*/  classRole,
    std::string const& platformId,
    std::string const& thirdPartyName,
    std::string const& platformOnlineId,
    std::string const& platformOfflineId,
    std::optional<class PlayerPartyInfo> const& partyInfo,
    bool compatibleWithClientSideChunkGen,
    struct SyncedClientOptionsComponent const& options,
    void* /*std::optional<class Nonce> const&*/  nonce
    )
{

    SerializedSkinImpl* skinImpl = (skinInfo && skinInfo->skin && skinInfo->skin->impl)
        ? &skinInfo->skin->impl->Object
        : nullptr;

    auto holder = nes::make_holder<ConnectionRequestEvent>(*selfSignedId, *serverAddress, clientRandomId, *skinInfo->skinId, *deviceId, currentInputMode, skinImpl);
    gFeatureManager->mDispatcher->trigger(holder);

    *selfSignedId = holder->mSelfSignedId;
    *serverAddress = holder->mServerAddress;
    clientRandomId = holder->mClientRandomId;
    *skinInfo->skinId = holder->mSkinId;
    *deviceId = holder->mDeviceId;
    currentInputMode = holder->mInputMode;

    auto oFunc = mDetour->getOriginal<&createRequestDetourFunc>();

    return oFunc(
        _this,
        appPlatform,
        authInfo,
        selfSignedId,
        serverAddress,
        clientRandomId,
        skinInfo,
        deviceId,
        currentInputMode,
        guiScale,
        languageCode,
        clientIsEditorCapable,
        clientEditorConnectionIntent,
        isEduMode,
        eduTokenChain,
        eduSessionToken,
        eduJoinerToHostNonce,
        classRole,
        platformId,
        thirdPartyName,
        platformOnlineId,
        platformOfflineId,
        partyInfo,
        compatibleWithClientSideChunkGen,
        options,
        nonce
    );
}

void ConnectionRequestHook::init()
{
    mDetour = std::make_unique<Detour>("ConnectionRequest::create", reinterpret_cast<void*>(SigManager::ConnectionRequest_create), reinterpret_cast<void*>(&createRequestDetourFunc));
}