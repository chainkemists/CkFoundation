#include "CkInputSource_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInput/CkInput_Log.h"
#include "CkInput/CkInputSource_Utils.h"

#include <Engine/LocalPlayer.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSource_Subsystem::
    PlayerControllerChanged(
        APlayerController* NewPlayerController)
    -> void
{
    Super::PlayerControllerChanged(NewPlayerController);

    DoCreateInputSource();
}

auto
    UCk_InputSource_Subsystem::
    Deinitialize()
    -> void
{
    if (ck::IsValid(_InputSource))
    {
        auto SourceToDestroy = FCk_Handle{_InputSource};
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(SourceToDestroy);
    }

    _InputSource = FCk_Handle_InputSource{};

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSource_Subsystem::
    Get_InputSource()
    -> FCk_Handle_InputSource
{
    DoCreateInputSource();

    return _InputSource;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSource_Subsystem::
    DoCreateInputSource()
    -> void
{
    // Same timing discipline as UCk_KeyBinding_Subsystem::DoBindToSettingsAndRegisterScanPaths: the engine
    // hands the local player its PlayerController AFTER the subsystem collection initializes, so this is
    // re-attempted from PlayerControllerChanged and lazily from Get_InputSource until it can succeed.
    if (ck::IsValid(_InputSource))
    { return; }

    const auto* LocalPlayer = GetLocalPlayer();
    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    if (const auto* PlayerController = LocalPlayer->GetPlayerController(World);
        ck::Is_NOT_Valid(PlayerController))
    { return; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(World);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(NewEntity, TEXT("InputSource"));
#endif

    const auto LocalPlayerIndex = LocalPlayer->GetLocalPlayerIndex();

    _InputSource = UCk_Utils_InputSource_UE::Add(NewEntity,
        FCk_Fragment_InputSource_ParamsData{LocalPlayerIndex});

    ck::input::Display(TEXT("Created InputSource entity [{}] for local player index [{}]"),
        _InputSource, LocalPlayerIndex);
}

// --------------------------------------------------------------------------------------------------------------------
