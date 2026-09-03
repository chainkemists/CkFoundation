#include "CkInputSource_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkInput/CkInput_Log.h"
#include "CkInput/CkInputButtonMap_Utils.h"
#include "CkInput/CkInputSource_Utils.h"

#include <EnhancedInputSubsystems.h>
#include <Engine/LocalPlayer.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>
#include <UObject/WeakObjectPtrTemplates.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSource_Subsystem::
    PlayerControllerChanged(
        APlayerController* NewPlayerController)
    -> void
{
    Super::PlayerControllerChanged(NewPlayerController);

    DoCreateInputSource();
    DoBindToSettings();
}

auto
    UCk_InputSource_Subsystem::
    Deinitialize()
    -> void
{
    if (_BoundToSettings)
    {
        if (const auto* LocalPlayer = GetLocalPlayer(); ck::IsValid(LocalPlayer))
        {
            if (auto* EISubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
                ck::IsValid(EISubsystem))
            {
                if (auto* Settings = EISubsystem->GetUserSettings();
                    ck::IsValid(Settings))
                {
                    // Both, because `_BoundToSettings` is ONE flag over BOTH binds — leaving the registration
                    // delegate on would make the flag's `false` a lie, and a re-initialized subsystem would then
                    // add a second registration binding beside the surviving one.
                    Settings->OnSettingsChanged.RemoveDynamic(this, &UCk_InputSource_Subsystem::OnSettingsChanged);
                    Settings->OnMappingContextRegistered.RemoveDynamic(
                        this, &UCk_InputSource_Subsystem::OnMappingContextRegistered);
                }
            }
        }

        _BoundToSettings = false;
    }

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
    DoBindToSettings();

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

    if (World->bIsTearingDown)
    { return; }

    const TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> EcsWorld =
        World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (EcsWorld.IsValid() && EcsWorld->Get_LoadHold() == ECk_EcsWorld_LoadHold::Teardown)
    { return; }

    if (const auto* PlayerController = LocalPlayer->GetPlayerController(World);
        ck::Is_NOT_Valid(PlayerController))
    { return; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(World);
    const auto IsValidNewEntity = ck::IsValid(NewEntity);
    CK_ENSURE_IF_NOT(IsValidNewEntity,
        TEXT("InputSource creation returned an invalid entity outside world teardown"))
    {}
    if (NOT IsValidNewEntity)
    { return; }

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(NewEntity, TEXT("InputSource"));
#endif

    const auto LocalPlayerIndex = LocalPlayer->GetLocalPlayerIndex();

    _InputSource = UCk_Utils_InputSource_UE::Add(NewEntity,
        FCk_Fragment_InputSource_ParamsData{LocalPlayerIndex});

    ck::input::Display(TEXT("Created InputSource entity [{}] for local player index [{}]"),
        _InputSource, LocalPlayerIndex);
}

auto
    UCk_InputSource_Subsystem::
    DoBindToSettings()
    -> void
{
    // Same timing discipline as DoCreateInputSource: UEnhancedInputUserSettings does not exist until the
    // engine has given the local player a controller, so this is re-attempted from the same two entry points
    // until it can succeed.
    if (_BoundToSettings)
    { return; }

    const auto* LocalPlayer = GetLocalPlayer();
    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* EISubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (ck::Is_NOT_Valid(EISubsystem))
    { return; }

    // Null here is either the transient not-ready-yet above or user settings being disabled project-wide,
    // which is permanent. The disabled case is diagnosed once by UCk_KeyBinding_Subsystem — the other
    // LocalPlayerSubsystem that comes up beside this one — so this early-out stays quiet on purpose
    // rather than firing a second ensure for the same misconfiguration.
    auto* Settings = EISubsystem->GetUserSettings();
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->OnSettingsChanged.AddDynamic(this, &UCk_InputSource_Subsystem::OnSettingsChanged);
    // Registration is the OTHER way new mapping names reach the key profile, and it does NOT fire
    // OnSettingsChanged — a runtime-built IMC registered after the map's first derive would otherwise
    // never mint its names, permanently starving any consumer gating on them (the second-engaged-station
    // bug: the compose loop spins on an unminted button with no diagnostic).
    Settings->OnMappingContextRegistered.AddDynamic(this, &UCk_InputSource_Subsystem::OnMappingContextRegistered);
    _BoundToSettings = true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSource_Subsystem::
    OnSettingsChanged(
        UEnhancedInputUserSettings* InSettings)
    -> void
{
    DoRequestRederive();
}

auto
    UCk_InputSource_Subsystem::
    OnMappingContextRegistered(
        const UInputMappingContext* InMappingContext)
    -> void
{
    DoRequestRederive();
}

auto
    UCk_InputSource_Subsystem::
    DoRequestRederive()
    -> void
{
    // The button map is opt-in, so a source without one has nothing to re-derive. The request is deferred like
    // every other one, which means a rebind made from gameplay or a settings widget first shows up in the map
    // on the next frame's collect pass.
    auto ButtonMap = UCk_Utils_InputButtonMap_UE::Cast(_InputSource);
    if (ck::Is_NOT_Valid(ButtonMap))
    { return; }

    UCk_Utils_InputButtonMap_UE::Request_Rederive(ButtonMap, FCk_Request_InputButtonMap_Rederive{}, {});
}

// --------------------------------------------------------------------------------------------------------------------
