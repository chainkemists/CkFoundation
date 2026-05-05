#include "CkGameSession_Subsystem.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"

#include <GameFramework/GameModeBase.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSession_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    if (NOT UCk_Utils_Game_UE::Get_IsInGame(this))
    { return; }

    _PostLoginDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_GameSession_Subsystem_UE::OnLoginEvent);
    _PostLogoutDelegateHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UCk_GameSession_Subsystem_UE::OnLogoutEvent);

    // Own a private FEcsWorld for the signal-host entity. Pre-migration this
    // worked implicitly because FCk_Registry's default ctor auto-allocated an
    // entt registry; post-migration we have to do it explicitly.
    _InternalEcsWorld = MakeUnique<ck::FEcsWorld>();
    _SignalHandle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_InternalEcsWorld->Get_Registry());
}

auto
    UCk_GameSession_Subsystem_UE::
    Deinitialize()
    -> void
{
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoginDelegateHandle);
    FGameModeEvents::GameModeLogoutEvent.Remove(_PostLogoutDelegateHandle);

    // Clear the signal host before tearing down the registry it lives in so
    // any outstanding subscriber resolves to nullptr cleanly via the slot table.
    _SignalHandle = FCk_Handle{};
    _InternalEcsWorld.Reset();

    Super::Deinitialize();
}

auto
    UCk_GameSession_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    const auto& ShouldCreateSubsystem = Super::ShouldCreateSubsystem(InOuter);

    if (NOT ShouldCreateSubsystem)
    { return false; }

    if (ck::Is_NOT_Valid(InOuter))
    { return true; }

    const auto& World = InOuter->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return true; }

    return Super::ShouldCreateSubsystem(InOuter);
}

auto
    UCk_GameSession_Subsystem_UE::
    OnLoginEvent(
        AGameModeBase*     InGameModeBase,
        APlayerController* InPlayerController)
    -> void
{
    _AllPlayerControllers.Emplace(InPlayerController);
    ck::UUtils_Signal_OnLoginEvent::Broadcast(_SignalHandle, ck::MakePayload(MakeWeakObjectPtr(InPlayerController), _AllPlayerControllers));
}

auto
    UCk_GameSession_Subsystem_UE::
    OnLogoutEvent(
        AGameModeBase* InGameModeBase,
        AController*   InPlayerController)
    -> void
{
    _AllPlayerControllers.Remove(Cast<APlayerController>(InPlayerController));
    ck::UUtils_Signal_OnLogoutEvent::Broadcast(_SignalHandle,
        ck::MakePayload(Cast<APlayerController>(InPlayerController), _AllPlayerControllers));
}

auto
    UCk_GameSession_Subsystem_UE::
    BindTo_OnLoginEvent(
        ECk_Signal_BindingPolicy         InBehavior,
        ECk_Signal_PostFireBehavior      InPostFireBehavior,
        const FCk_Delegate_OnLoginEvent& InDelegate)
    -> void
{
    if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
    { ck::UUtils_Signal_OnLoginEvent::Bind(_SignalHandle, InDelegate, InBehavior); }
    else
    { ck::UUtils_Signal_OnLoginEvent_PostFireUnbind::Bind(_SignalHandle, InDelegate, InBehavior); }
}

auto
    UCk_GameSession_Subsystem_UE::
    BindTo_OnLogoutEvent(
        ECk_Signal_BindingPolicy          InBehavior,
        ECk_Signal_PostFireBehavior       InPostFireBehavior,
        const FCk_Delegate_OnLogoutEvent& InDelegate)
    -> void
{
    if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
    { ck::UUtils_Signal_OnLogoutEvent::Bind(_SignalHandle, InDelegate, InBehavior); }
    else
    { ck::UUtils_Signal_OnLogoutEvent_PostFireUnbind::Bind(_SignalHandle, InDelegate, InBehavior); }
}

auto
    UCk_GameSession_Subsystem_UE::
    Unbind_OnLoginEvent(
        const FCk_Delegate_OnLoginEvent& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnLoginEvent::Unbind(_SignalHandle, InDelegate);
    ck::UUtils_Signal_OnLoginEvent_PostFireUnbind::Unbind(_SignalHandle, InDelegate);
}

auto
    UCk_GameSession_Subsystem_UE::
    Unbind_OnLogoutEvent(
        const FCk_Delegate_OnLogoutEvent& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnLogoutEvent::Unbind(_SignalHandle, InDelegate);
    ck::UUtils_Signal_OnLogoutEvent_PostFireUnbind::Unbind(_SignalHandle, InDelegate);
}

auto
    UCk_GameSession_Subsystem_UE::
    Get_AllPlayerControllers() const
    -> TArray<APlayerController*>
{
    return ck::algo::Transform<TArray<APlayerController*>>(_AllPlayerControllers, [](TWeakObjectPtr<APlayerController> InPC)
    {
        return InPC.Get();
    });
}

// --------------------------------------------------------------------------------------------------------------------
