#include "CkSnapshot_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkEcsExt/OwningActor/CkActorRebind_Utils.h"

#include "CkSnapshot/SaveKey/CkSnapshot_SaveKey_Fragment.h"
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
    Request_AssignSaveKey(
        FCk_Handle& InHandle,
        const FString& InStableIdentity)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Cannot assign the SaveKey [{}] — the Entity Handle is invalid"), InStableIdentity)
    {}
    if (NOT HandleIsValid)
    { return; }

    // An empty identity hashes to a single shared GUID, so every empty-keyed entity would rendezvous onto the same
    // one and silently consolidate. Reject rather than key.
    const auto IdentityIsValid = NOT InStableIdentity.IsEmpty();
    CK_ENSURE_IF_NOT(IdentityIsValid,
        TEXT("Cannot assign a SaveKey to Entity [{}] — the stable identity is EMPTY"), InHandle)
    {}
    if (NOT IdentityIsValid)
    { return; }

    InHandle.AddOrReplace<FFragment_SaveKey>(FGuid::NewDeterministicGuid(InStableIdentity));
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
