#include "CkSnapshot_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcsExt/OwningActor/CkActorRebind_Utils.h"

#include "CkSnapshot/Subsystem/CkSnapshot_Subsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_WasJustRestored(
        const FCk_Handle& InHandle)
    -> bool
{
    // An invalid handle is a legitimate ask with answer "no" (callers poll before composition settles), not a
    // contract violation — silent false, no ensure.
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FTag_Snapshot_JustRestored>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_WasActorJustRebound(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FTag_ActorJustRebound>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_IsLoadInProgress(
        const FCk_Handle& InHandle)
    -> bool
{
    // Invalid-handle guard only. The transient entity deliberately falls through to Get_WorldForEntity's own
    // ensure — this query needs a real entity to resolve a world from, and a caller handing it the transient
    // has a bug worth hearing about.
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    const auto GameInstance = World->GetGameInstance();
    if (ck::Is_NOT_Valid(GameInstance, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    const auto Subsystem = GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return Subsystem->Get_IsLoadInProgress();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Promise_OnLoadComplete(
        const FCk_Handle& InAnyWorldHandle,
        const FCk_Delegate_Snapshot_OnLoadComplete& InDelegate)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InAnyWorldHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Promise_OnLoadComplete requires a valid world-resolving handle"))
    {}
    if (NOT HandleIsValid)
    { return; }

    const auto FireImmediately = [&]() -> void
    {
        InDelegate.ExecuteIfBound(InAnyWorldHandle, FCk_Snapshot_LoadReport{});
    };

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyWorldHandle);
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
    { FireImmediately(); return; }

    const auto GameInstance = World->GetGameInstance();
    const auto Subsystem = ck::IsValid(GameInstance, ck::IsValid_Policy_NullptrOnly{})
        ? GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>()
        : nullptr;
    if (ck::Is_NOT_Valid(Subsystem, ck::IsValid_Policy_NullptrOnly{}) || NOT Subsystem->Get_IsLoadInProgress())
    { FireImmediately(); return; }

    // IgnorePayloadInFlight is load-bearing: an earlier load's completion payload may still be in flight on the
    // signal, and a replay-on-bind policy would fire THIS promise immediately with the OLD load's report while
    // the current load is still reconstituting the world — the exact half-coherent read this API exists to prevent.
    auto Source = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);
    CK_SIGNAL_BIND(ck::UUtils_Signal_Snapshot_OnLoadComplete, Source, InDelegate,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::Unbind);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_AssignSaveKey(
        FCk_Handle& InHandle,
        const FString& InStableIdentity)
    -> void
{
    ck::save_key::Assign(InHandle, InStableIdentity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_AssignSaveKeyAlias(
        FCk_Handle& InHandle,
        const FString& InHistoricalIdentity)
    -> void
{
    ck::save_key::AssignAlias(InHandle, InHistoricalIdentity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_HasSaveKey(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<FFragment_SaveKey>();
}
