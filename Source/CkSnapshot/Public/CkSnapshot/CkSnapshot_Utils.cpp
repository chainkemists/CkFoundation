#include "CkSnapshot_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkSnapshot/SaveGame/CkSnapshot_SlotMeta.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Subsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_utils
{
    auto
    DoGet_Subsystem(
        const FCk_Handle& InHandle)
    -> UCk_Snapshot_Subsystem_UE*
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto GameInstance = ck::IsValid(World) ? World->GetGameInstance() : nullptr;
        return ck::IsValid(GameInstance)
            ? GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>()
            : nullptr;
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    DoMakeFailedPersistentMutationTicket(
        ECk_PersistentEntityMutationResult InResult)
    -> FCk_PersistentEntityMutationTicket
{
    auto Ticket = FCk_PersistentEntityMutationTicket{};
    Ticket._BeginResult = InResult;
    return Ticket;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_WasHydratedThisLoad(
        const FCk_Handle& InHandle)
    -> bool
{
    // An invalid handle is a legitimate ask with answer "no" (callers poll before composition settles), not a
    // contract violation — silent false, no ensure.
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FTag_Hydration_WasHydratedThisLoad>();
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
    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto GameInstance = World->GetGameInstance();
    if (ck::Is_NOT_Valid(GameInstance))
    { return false; }

    const auto Subsystem = GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>();
    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Get_IsLoadInProgress();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Promise_OnLoadComplete(
        const FCk_Handle& InAnyWorldHandle,
        const FCk_Delegate_Snapshot_OnLoadComplete& InDelegate)
    -> ECk_Snapshot_PromiseResult
{
    const auto HandleIsValid = ck::IsValid(InAnyWorldHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Promise_OnLoadComplete requires a valid world-resolving handle"))
    { return ECk_Snapshot_PromiseResult::NoLoadInProgress; }

    // Nothing was loading, so the world is already as coherent as it is going to get and the answer is due NOW.
    // The report says exactly that instead of the default-constructed Failed_IO it used to carry, which reported a
    // failure for an operation that never happened.
    const auto FireImmediately = [&]() -> ECk_Snapshot_PromiseResult
    {
        auto Report = FCk_Snapshot_LoadReport{};
        Report.Set_Result(ECk_SnapshotResult::NoLoadInProgress);
        InDelegate.ExecuteIfBound(InAnyWorldHandle, Report);
        return ECk_Snapshot_PromiseResult::NoLoadInProgress;
    };

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyWorldHandle);
    if (ck::Is_NOT_Valid(World))
    { return FireImmediately(); }

    const auto GameInstance = World->GetGameInstance();
    const auto Subsystem = ck::IsValid(GameInstance)
        ? GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>()
        : nullptr;
    if (ck::Is_NOT_Valid(Subsystem) || NOT Subsystem->Get_IsLoadInProgress())
    { return FireImmediately(); }

    // Queued on the SUBSYSTEM, not bound on the world's transient entity. A load travels, and every ECS registry
    // is world-scoped, so the old bind lived on a world the load was about to tear down: a promise made before
    // the travel — which is most of them, since a load is exactly when consumers reach for this — was dropped on
    // the floor with nothing to say so. The subsystem is GameInstance-scoped and survives the travel intact.
    Subsystem->Request_AddLoadCompletePromise(InDelegate);
    return ECk_Snapshot_PromiseResult::Bound;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_IsRebuildInProgress(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return false; }

    // Every phase, not just the rebuild ones. A seed produced while payloads drain, or while the world converges,
    // duplicates the restored copy exactly as thoroughly as one produced under the kernel — the difference is
    // only that by then the copy it duplicates is already visible.
    return EcsWorld->Get_LoadHold() != ECk_EcsWorld_LoadHold::None;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Promise_OnHydrated(
        const FCk_Handle& InHandle,
        const FCk_Delegate_Hydration_OnHydrated& InDelegate)
    -> void
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Promise_OnHydrated requires a valid entity handle"))
    { return; }

    const auto FireImmediately = [&]() -> void
    {
        InDelegate.ExecuteIfBound(InHandle);
    };

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    if (ck::Is_NOT_Valid(World))
    { FireImmediately(); return; }

    const auto GameInstance = World->GetGameInstance();
    const auto Subsystem = ck::IsValid(GameInstance)
        ? GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>()
        : nullptr;

    if (ck::Is_NOT_Valid(Subsystem))
    { FireImmediately(); return; }

    // A mapped entity has a definite lift edge. Bind to it directly rather than sending the promise through the
    // subsystem's pre-map queue.
    if (Subsystem->Get_IsHydrationPending(InHandle))
    {
        auto Source = InHandle;
        CK_SIGNAL_BIND(ck::UUtils_Signal_Hydration_OnHydrated, Source, InDelegate,
            ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
            ECk_Signal_PostFireBehavior::Unbind);
        return;
    }

    // Before rebuild has mapped this handle, false does not mean the entity is fresh: it is also the unavoidable
    // pre-map interval. Let the GameInstance subsystem carry that ambiguity until mapping completes. It transfers
    // a mapped promise to the ordinary one-shot lift signal, or executes a never-mapped promise at load completion.
    if (Subsystem->Request_AddHydrationPromise(InHandle, InDelegate))
    { return; }

    // No load can still map this entity: a fresh spawn, client, no load in flight, or a quarantine already lifted.
    FireImmediately();
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
    Make_PersistentEntityAuthorityReference(
        const FCk_Handle& InEntity)
    -> FCk_PersistentEntityAuthorityReference
{
    auto Reference = FCk_PersistentEntityAuthorityReference{};
    if (ck::Is_NOT_Valid(InEntity))
    { return Reference; }

    Reference._NetworkHandle = InEntity;
    if (InEntity.Has<FFragment_SaveKey>())
    {
        const auto& SaveKey = InEntity.Get<FFragment_SaveKey>();
        if (SaveKey.Get_IsLevelPlacedRoot() && NOT SaveKey.Get_IsSharedRendezvousGroup())
        { Reference._AuthoredSaveKey = SaveKey.Get_Key(); }
    }
    return Reference;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Resolve_PersistentEntityAuthorityReference(
        const FCk_Handle& InWorldContext,
        const FCk_PersistentEntityAuthorityReference& InReference)
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(InWorldContext))
    { return {}; }

    auto* ExpectedWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InWorldContext);
    if (ck::Is_NOT_Valid(ExpectedWorld))
    { return {}; }

    if (ck::IsValid(InReference._NetworkHandle) &&
        UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InReference._NetworkHandle) == ExpectedWorld)
    {
        if (NOT InReference._AuthoredSaveKey.IsValid())
        { return InReference._NetworkHandle; }

        if (InReference._NetworkHandle.Has<FFragment_SaveKey>() &&
            InReference._NetworkHandle.Get<FFragment_SaveKey>().Get_Key() == InReference._AuthoredSaveKey)
        { return InReference._NetworkHandle; }

        return {};
    }

    if (NOT InReference._AuthoredSaveKey.IsValid())
    { return {}; }

    const auto* GameInstance = ExpectedWorld->GetGameInstance();
    auto* Subsystem = ck::IsValid(GameInstance) ? GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>() : nullptr;
    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    auto Resolved = FCk_Handle{};
    if (NOT Subsystem->TryResolve_SaveKey(InReference._AuthoredSaveKey, Resolved))
    { return {}; }
    return Resolved;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_BeginEntityRemoval(
        const FCk_Handle& InSource)
    -> FCk_PersistentEntityMutationTicket
{
    const auto SourceIsValid = ck::IsValid(InSource);
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("Persistent entity removal requires a valid source entity"))
    { }
    if (NOT SourceIsValid)
    { return DoMakeFailedPersistentMutationTicket(ECk_PersistentEntityMutationResult::Failed_InvalidSource); }

    const auto Subsystem = ck_snapshot_utils::DoGet_Subsystem(InSource);
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Persistent entity removal source [{}] has no snapshot subsystem"), InSource)
    { }
    if (NOT HasSubsystem)
    { return DoMakeFailedPersistentMutationTicket(ECk_PersistentEntityMutationResult::Failed_WorldUnavailable); }

    return Subsystem->Request_BeginEntityRemoval(InSource);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_CommitEntityRemoval(
        const FCk_PersistentEntityMutationTicket& InTicket)
    -> ECk_PersistentEntityMutationResult
{
    const auto Subsystem = InTicket._OwningSubsystem.Get();
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Persistent entity removal commit requires a live mutation ticket"))
    { }
    if (NOT HasSubsystem)
    { return ECk_PersistentEntityMutationResult::Failed_InvalidTicket; }

    return Subsystem->Request_CommitEntityRemoval(InTicket);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_CancelPersistentMutation(
        const FCk_PersistentEntityMutationTicket& InTicket)
    -> ECk_PersistentEntityMutationResult
{
    const auto Subsystem = InTicket._OwningSubsystem.Get();
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Persistent mutation cancellation requires a live mutation ticket"))
    { }
    if (NOT HasSubsystem)
    { return ECk_PersistentEntityMutationResult::Failed_InvalidTicket; }

    return Subsystem->Request_CancelPersistentMutation(InTicket);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_DestroyEntityPersistently(
        const FCk_Handle& InSource)
    -> ECk_PersistentEntityMutationResult
{
    const auto SourceIsValid = ck::IsValid(InSource);
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("Persistent entity destruction requires a valid source entity"))
    { }
    if (NOT SourceIsValid)
    { return ECk_PersistentEntityMutationResult::Failed_InvalidSource; }

    const auto Subsystem = ck_snapshot_utils::DoGet_Subsystem(InSource);
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Persistent entity destruction source [{}] has no snapshot subsystem"), InSource)
    { }
    if (NOT HasSubsystem)
    { return ECk_PersistentEntityMutationResult::Failed_WorldUnavailable; }

    return Subsystem->Request_DestroyEntityPersistently(InSource);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_BeginPersistentRelocation(
        const FCk_Handle& InSource)
    -> FCk_PersistentEntityMutationTicket
{
    const auto SourceIsValid = ck::IsValid(InSource);
    CK_ENSURE_IF_NOT(SourceIsValid,
        TEXT("Persistent relocation requires a valid source entity"))
    { }
    if (NOT SourceIsValid)
    { return DoMakeFailedPersistentMutationTicket(ECk_PersistentEntityMutationResult::Failed_InvalidSource); }

    const auto Subsystem = ck_snapshot_utils::DoGet_Subsystem(InSource);
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Persistent relocation source [{}] has no snapshot subsystem"), InSource)
    { }
    if (NOT HasSubsystem)
    { return DoMakeFailedPersistentMutationTicket(ECk_PersistentEntityMutationResult::Failed_WorldUnavailable); }

    return Subsystem->Request_BeginPersistentRelocation(InSource);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_CompletePersistentRelocation(
        FCk_Handle& InDestination,
        const FCk_PersistentEntityMutationTicket& InTicket)
    -> ECk_PersistentEntityMutationResult
{
    const auto Subsystem = InTicket._OwningSubsystem.Get();
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Persistent relocation completion requires a live mutation ticket"))
    { }
    if (NOT HasSubsystem)
    { return ECk_PersistentEntityMutationResult::Failed_InvalidTicket; }

    return Subsystem->Request_CompletePersistentRelocation(InDestination, InTicket);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_CompleteLegacySaveKeyRelocation(
        FCk_Handle& InDestination,
        const FGuid& InSaveKey)
    -> ECk_PersistentEntityMutationResult
{
    const auto DestinationIsValid = ck::IsValid(InDestination);
    CK_ENSURE_IF_NOT(DestinationIsValid,
        TEXT("Legacy SaveKey relocation requires a valid destination entity"))
    { }
    if (NOT DestinationIsValid)
    { return ECk_PersistentEntityMutationResult::Failed_InvalidDestination; }

    const auto Subsystem = ck_snapshot_utils::DoGet_Subsystem(InDestination);
    const auto HasSubsystem = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(HasSubsystem,
        TEXT("Legacy SaveKey relocation destination [{}] has no snapshot subsystem"), InDestination)
    { }
    if (NOT HasSubsystem)
    { return ECk_PersistentEntityMutationResult::Failed_WorldUnavailable; }

    return Subsystem->Request_CompleteLegacySaveKeyRelocation(InDestination, InSaveKey);
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_SlotScreenshotTexture(
        const FCk_Snapshot_SlotMeta& InMeta)
    -> UTexture2D*
{
    return ck::snapshot::slot_meta::Decode_PngAsTexture(InMeta.Get_ScreenshotPng());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_IsSlotMetaPopulated(
        const FCk_Snapshot_SlotMeta& InMeta)
    -> bool
{
    return InMeta.Get_IsPopulated();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_SlotCustomField(
        const FCk_Snapshot_SlotMeta& InMeta,
        FName InKey,
        const FString& InFallback)
    -> FString
{
    const auto* Found = InMeta.Get_CustomFields().Find(InKey);

    return ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}) ? *Found : InFallback;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Request_CaptureViewportThumbnail(
        const UObject* InWorldContextObject,
        const FCk_Delegate_Snapshot_OnThumbnailCaptured& InDelegate,
        int32 InMaxWidth)
    -> void
{
    const auto* World = ck::IsValid(InWorldContextObject)
        ? GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull)
        : nullptr;

    ck::snapshot::slot_meta::Request_CaptureViewportPng(World, InMaxWidth,
        [Delegate = InDelegate](TArray<uint8> InPng) -> void
        {
            Delegate.ExecuteIfBound(InPng);
        });
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_utils
{
    auto TryGet_SnapshotSubsystem(
        const UObject* InWorldContextObject) -> UCk_Snapshot_Subsystem_UE*
    {
        if (ck::Is_NOT_Valid(InWorldContextObject))
        { return nullptr; }

        const auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject,
            EGetWorldErrorMode::ReturnNull);

        if (World == nullptr)
        { return nullptr; }

        auto* GameInstance = World->GetGameInstance();
        if (ck::Is_NOT_Valid(GameInstance))
        { return nullptr; }

        return GameInstance->GetSubsystem<UCk_Snapshot_Subsystem_UE>();
    }
}

auto
    UCk_Utils_Snapshot_UE::
    Get_IsReadyToResume(
        const UObject* InWorldContextObject)
    -> bool
{
    const auto* Subsystem = ck_snapshot_utils::TryGet_SnapshotSubsystem(InWorldContextObject);

    // No subsystem is the same answer as no load: there is nothing holding this world. A menu world and a
    // dedicated server both land here, and both are running.
    if (ck::Is_NOT_Valid(Subsystem))
    { return true; }

    // A world that has never loaded is running, and saying otherwise would make every consumer special-case the
    // ordinary case. The subsystem's own flag answers strictly about THIS load, so the never-loaded world is
    // resolved here rather than there.
    if (NOT Subsystem->Get_IsLoadInProgress())
    { return true; }

    return Subsystem->Get_IsReadyToResume();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_LoadEpoch(
        const UObject* InWorldContextObject)
    -> int32
{
    const auto* Subsystem = ck_snapshot_utils::TryGet_SnapshotSubsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Get_LoadEpoch();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Snapshot_UE::
    Get_DidLoadComplete(
        const FCk_Snapshot_LoadReport& InReport)
    -> bool
{
    return InReport.Get_DidLoadComplete();
}

// --------------------------------------------------------------------------------------------------------------------
