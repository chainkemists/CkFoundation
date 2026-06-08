#include "CkSnapshot_Subsystem.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/SaveGame/CkSnapshot_SaveGame.h"
#include "CkSnapshot/Snapshot/CkSnapshot_Capture.h"
#include "CkSnapshot/Snapshot/CkSnapshot_Restore.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Signals.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"                       // ck::FTag_DestroyEntity_*, FFragment_LifetimeDependents
#include "CkEcs/Handle/CkHandle_Utils.h"                 // M2b: ck::MakeHandle
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h" // M2b: respawn intent
#include "CkEcsExt/OwningActor/CkActorRespawn_Fragment.h"     // M2b-2a: FTag_ActorRespawn_Pending marker
#include "CkEcsExt/Transform/CkTransform_Utils.h"             // M2b position-restore: restored spawn transform

#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

#include <Engine/World.h>
#include <GameFramework/Actor.h>   // M2b: SpawnActor<AActor> needs the complete type
#include <GameFramework/GameModeBase.h>   // M2b-2b: AGameModeBase::bUseSeamlessTravel for seamless ServerTravel
#include "UObject/SoftObjectPath.h" // M2b: FSoftClassPath::TryLoadClass for the respawn pass

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_subsystem
{
    constexpr auto UserIndex = 0;

    auto
        DoGet_HasWorldAuthority(
            const UWorld* InWorld)
        -> bool
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return false; }

        return InWorld->GetNetMode() != ENetMode::NM_Client;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_SnapshotSource() const
    -> FCk_Handle
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return {}; }

    return UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Snapshot_Subsystem_UE::
    Get_IsLoadInProgress() const
{
    return _LoadInProgress;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    Deinitialize()
    -> void
{
    // The world/game-instance is going away mid-load: drop the ticker so the callback never fires into a dead subsystem.
    if (_LoadTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_LoadTickerHandle);
        _LoadTickerHandle.Reset();
    }
    DoSet_ReconstitutionFlag(false); // clear the world flag if we tear down mid-load
    _LoadInProgress = false;
    _LoadPhase = ELoadPhase::Idle;

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    Request_Save(
        FName InSlotName,
        const FCk_Delegate_OnSaveComplete& InDelegate)
{
    const auto World = GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World) && ck_snapshot_subsystem::DoGet_HasWorldAuthority(World),
        TEXT("Request_Save refused: World [{}] is invalid or this is a client (no authority)"), World)
    {
        InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_IO);
        return;
    }

    CK_ENSURE_IF_NOT(NOT _SnapshotInProgress && NOT _LoadInProgress,
        TEXT("Request_Save refused: a snapshot operation is already in progress"))
    {
        InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_IO);
        return;
    }

    _SnapshotInProgress = true;
    ON_SCOPE_EXIT { _SnapshotInProgress = false; };

    auto Source = DoGet_SnapshotSource();
    ck::UUtils_Signal_Snapshot_OnPreSave::Broadcast(Source, ck::MakePayload(Source));

    // Settle the world so the capture reflects a consistent, quiescent state. The scheduler already loops
    // its own DoPump to quiescence; we just drive a DeltaTime=0 pass across every ticking group.
    if (auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        ck::IsValid(EcsWorld))
    {
        _LastPumpCount = EcsWorld->Request_PumpToQuiescence();
        ck::snapshot::Verbose(TEXT("Request_Save: pumped world to quiescence in [{}] pump passes"), _LastPumpCount);
    }

    auto ByteWriter = FBufferArchive{};
    auto Header = FCk_Snapshot_Header{};

    const auto CaptureResult = ck::snapshot::Run_Capture(*World, ByteWriter, Header);

    auto DoFinish = [&](ECk_SnapshotResult InResult) -> void
    {
        ck::UUtils_Signal_Snapshot_OnSaveComplete::Broadcast(Source, ck::MakePayload(Source, InResult));
        InDelegate.ExecuteIfBound(InResult);
    };

    if (CaptureResult != ECk_SnapshotResult::Success)
    {
        ck::snapshot::Error(TEXT("Request_Save: capture failed with result [{}]"), CaptureResult);
        DoFinish(CaptureResult);
        return;
    }

    auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SaveGame::StaticClass()));
    CK_ENSURE_IF_NOT(ck::IsValid(SaveGame),
        TEXT("Request_Save: failed to create UCk_Snapshot_SaveGame"))
    {
        DoFinish(ECk_SnapshotResult::Failed_IO);
        return;
    }

    SaveGame->_Header = Header;
    SaveGame->_SnapshotBytes = MoveTemp(static_cast<TArray<uint8>&>(ByteWriter));

    const auto Saved = UGameplayStatics::SaveGameToSlot(SaveGame, InSlotName.ToString(), ck_snapshot_subsystem::UserIndex);
    if (NOT Saved)
    {
        ck::snapshot::Error(TEXT("Request_Save: SaveGameToSlot failed for slot [{}]"), InSlotName);
        DoFinish(ECk_SnapshotResult::Failed_IO);
        return;
    }

    ck::snapshot::Display(TEXT("Request_Save: saved [{}] bytes to slot [{}]"), SaveGame->_SnapshotBytes.Num(), InSlotName);
    DoFinish(ECk_SnapshotResult::Success);
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    Request_Load(
        FName InSlotName,
        const FCk_Delegate_OnLoadComplete& InDelegate)
{
    const auto World = GetWorld();

    auto MakeFailureReport = [&](ECk_SnapshotResult InResult) -> FCk_Snapshot_LoadReport
    {
        auto Report = FCk_Snapshot_LoadReport{};
        Report.Set_Result(InResult);
        return Report;
    };

    CK_ENSURE_IF_NOT(ck::IsValid(World) && ck_snapshot_subsystem::DoGet_HasWorldAuthority(World),
        TEXT("Request_Load refused: World [{}] is invalid or this is a client (no authority)"), World)
    {
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IO));
        return;
    }

    CK_ENSURE_IF_NOT(NOT _SnapshotInProgress && NOT _LoadInProgress,
        TEXT("Request_Load refused: a snapshot operation is already in progress"))
    {
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IO));
        return;
    }

    auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(UGameplayStatics::LoadGameFromSlot(InSlotName.ToString(), ck_snapshot_subsystem::UserIndex));
    if (ck::Is_NOT_Valid(SaveGame))
    {
        ck::snapshot::Error(TEXT("Request_Load: no/invalid save in slot [{}]"), InSlotName);
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IO));
        return;
    }

    if (SaveGame->_Header.Get_FormatVersion() != 1)
    {
        ck::snapshot::Error(TEXT("Request_Load: incompatible format version [{}] in slot [{}]"),
            SaveGame->_Header.Get_FormatVersion(), InSlotName);
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IncompatibleSave));
        return;
    }

    // ---- Latch the load (spans real frames + a level reload from here) -------------------------------------
    _LoadInProgress      = true;
    _LoadPhase           = ELoadPhase::TearingDown;
    _PendingLoadBytes    = SaveGame->_SnapshotBytes;   // copy — the SaveGame object is not kept alive across frames
    _PendingLoadHeader   = SaveGame->_Header;
    _PendingLoadDelegate = InDelegate;
    _LoadFrameCount      = 0;
    _PreTravelWorld      = nullptr;
    _TravelMapName.Reset();
    _RespawnQuiescenceFramesRemaining = 0;
    _RespawnQuiescenceStarted = false;

    // Mark THIS world as reconstituting so any bridged actor torn down + any straggler construct abstains.
    DoSet_ReconstitutionFlag(true);

    auto Source = DoGet_SnapshotSource();
    ck::UUtils_Signal_Snapshot_OnPreLoad::Broadcast(Source, ck::MakePayload(Source));

    DoInitiate_Teardown();

    _LoadTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_Snapshot_Subsystem_UE::DoTick_Load));

    ck::snapshot::Display(TEXT("Request_Load: load started for slot [{}] ([{}] roots tearing down)"),
        InSlotName, _PendingTeardownRoots.Num());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoInitiate_Teardown()
    -> void
{
    _PendingTeardownRoots.Reset();

    const auto Transient = DoGet_SnapshotSource();
    if (ck::Is_NOT_Valid(Transient) || NOT Transient.Has<ck::FFragment_LifetimeDependents>())
    {
        // Nothing to tear down (empty world) — completion poll will pass immediately.
        return;
    }

    // Copy the roots BEFORE destroying — Request_DestroyEntity mutates the dependents list.
    _PendingTeardownRoots = Transient.Get<ck::FFragment_LifetimeDependents>().Get_Entities();

    for (auto& Root : _PendingTeardownRoots)
    {
        if (ck::IsValid(Root))
        { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Root); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_TeardownComplete() const
    -> bool
{
    // Complete when every gameplay root we requested destroyed has finalized (handle now invalid). The root
    // finalizes only AFTER its whole subtree's cascade — incl. EndPlay — has run, so this also covers children.
    // We deliberately do NOT additionally require the FTag_DestroyEntity_* storages to be globally empty: a
    // continuously-ticking world always has some framework scratch/deferred entity mid-destruction, so that
    // global clause never settles. Any such straggler is wiped by Run_Restore's clear() immediately after.
    for (const auto& Root : _PendingTeardownRoots)
    {
        if (ck::IsValid(Root))
        { return false; }
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoSet_ReconstitutionFlag(
        bool InInProgress)
    -> void
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    if (auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        ck::IsValid(EcsWorld))
    {
        EcsWorld->Set_ReconstitutionInProgress(InInProgress);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoInitiate_Travel()
    -> void
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    _PreTravelWorld = World;
    _TravelMapName  = World->RemovePIEPrefix(World->GetOutermost()->GetName());

    constexpr auto AbsoluteTravel = true;

    // A true single-player game (NM_Standalone) reloads via OpenLevel — there are no clients to bring along.
    if (World->GetNetMode() == ENetMode::NM_Standalone)
    {
        ck::snapshot::Display(TEXT("DIAG: Request_Load travel — OpenLevel (Standalone) to map [{}] (pre-travel world [{}])"),
            _TravelMapName, World->GetName());
        UGameplayStatics::OpenLevel(World, FName{*_TravelMapName}, AbsoluteTravel);
        return;
    }

    // A server (listen/dedicated) must stay a server across the reload AND keep its connected clients. A HARD
    // ServerTravel cannot carry clients in one-process PIE (net-driver teardown/reconnect race — proven by spike),
    // so we use SEAMLESS travel: it preserves the UNetConnection across the world swap, so the engine carries each
    // client along (PlayerController/PlayerState persist; pawns are re-created). "?listen" keeps the server a
    // listen server. The server restores into the post-travel world (M2b-2a path); clients re-derive the restored
    // world via replication over the preserved connection.
    if (auto* GameMode = World->GetAuthGameMode();
        ck::IsValid(GameMode))
    {
        GameMode->bUseSeamlessTravel = true;
    }

    ck::snapshot::Display(TEXT("DIAG: Request_Load travel — seamless ServerTravel (netmode [{}]) to map [{}] (pre-travel world [{}])"),
        static_cast<int32>(World->GetNetMode()), _TravelMapName, World->GetName());

    World->ServerTravel(_TravelMapName + TEXT("?listen"), AbsoluteTravel);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_NewWorldReady() const
    -> bool
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    // A different world object than the one we left (both OpenLevel and seamless ServerTravel produce a fresh
    // world; detect by instance + HasBegunPlay, netmode-agnostic).
    if (World == _PreTravelWorld.Get())
    { return false; }

    // Seamless travel transits through an intermediate TRANSITION map before the destination map. We reload the
    // SAME map, so the destination's package name equals _TravelMapName; the transition map has a different name
    // and must be skipped (restoring into it would be wrong). For the OpenLevel/Standalone path there is no
    // transition map and the reloaded world's name also equals _TravelMapName, so this filter is uniformly safe.
    if (World->RemovePIEPrefix(World->GetOutermost()->GetName()) != _TravelMapName)
    { return false; }

    return World->HasBegunPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRun_Restore()
    -> FCk_Snapshot_LoadReport
{
    const auto World = GetWorld();
    auto Reader = FMemoryReader{_PendingLoadBytes, /*bIsPersistent=*/true};
    return ck::snapshot::Run_Restore(*World, Reader, _PendingLoadHeader);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoStamp_RespawnMarkers()
    -> int32
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return 0; }

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return 0; }

    auto& CkRegistry = EcsWorld->Get_Registry();
    auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
    if (RawRegistry == nullptr)
    { return 0; }

    // Stamp every restored bridged entity so FProcessor_ActorRespawn (scheduler tick) respawns + re-bridges it.
    // Collect first (Add mutates storages the view is over), then stamp.
    auto BridgedEntities = TArray<FCk_Handle>{};
    for (const auto Entity : RawRegistry->view<FFragment_ActorSpawnIntent>())
    {
        BridgedEntities.Add(ck::MakeHandle(FCk_Entity{Entity}, CkRegistry));
    }

    auto StampedCount = 0;
    for (auto& Entity : BridgedEntities)
    {
        if (ck::Is_NOT_Valid(Entity))
        { continue; }

        Entity.Add<ck::FTag_ActorRespawn_Pending>();
        ++StampedCount;
    }

    ck::snapshot::Display(TEXT("DIAG: stamped [{}] restored bridged entities for actor-respawn"), StampedCount);
    return StampedCount;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_RespawnComplete() const
    -> bool
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return true; } // no world → nothing to wait on

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return true; }

    auto& CkRegistry = EcsWorld->Get_Registry();
    auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
    if (RawRegistry == nullptr)
    { return true; }

    // Complete once the processor has consumed every marker. The marker tag's storage uses in_place deletion
    // (so the processor may Clear it mid-iteration) → entt SFINAE-disables view::empty() for that policy. Iterate
    // instead: the view skips tombstones, so any yielded entity means a marker is still pending.
    for (const auto Entity : RawRegistry->view<ck::FTag_ActorRespawn_Pending>())
    {
        (void)Entity;
        return false;
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoTick_Load(float /*InDeltaSeconds*/)
    -> bool
{
    ++_LoadFrameCount;

    switch (_LoadPhase)
    {
        case ELoadPhase::TearingDown:
        {
            if (NOT DoIs_TeardownComplete())
            {
                if (_LoadFrameCount >= kLoad_TeardownFrameCap)
                {
                    ck::snapshot::Error(TEXT("Request_Load: teardown did not drain within [{}] frames — aborting"),
                        kLoad_TeardownFrameCap);
                    auto Report = FCk_Snapshot_LoadReport{};
                    Report.Set_Result(ECk_SnapshotResult::Failed_IO);
                    DoFinish_Load(Report);
                    return false;
                }
                return true; // keep waiting
            }

            ck::snapshot::Display(TEXT("DIAG: teardown drained after [{}] frames — initiating travel"), _LoadFrameCount);
            DoInitiate_Travel();
            _LoadFrameCount = 0;
            _LoadPhase = ELoadPhase::AwaitingWorld;
            return true;
        }

        case ELoadPhase::AwaitingWorld:
        {
            if (NOT DoIs_NewWorldReady())
            {
                if (_LoadFrameCount >= kLoad_TravelFrameCap)
                {
                    ck::snapshot::Error(TEXT("Request_Load: post-travel world not ready within [{}] frames — aborting"),
                        kLoad_TravelFrameCap);
                    auto Report = FCk_Snapshot_LoadReport{};
                    Report.Set_Result(ECk_SnapshotResult::Failed_IO);
                    DoFinish_Load(Report);
                    return false;
                }
                return true;
            }

            ck::snapshot::Display(TEXT("DIAG: post-travel world ready after [{}] frames — restoring"), _LoadFrameCount);
            // The new world's EcsWorld subsystem started fresh (flag default false) — set it so the respawn pass's
            // spawned actors abstain in their own (deferred) WithActor::Construct.
            DoSet_ReconstitutionFlag(true);
            _LoadPhase = ELoadPhase::Restoring;
            return true;
        }

        case ELoadPhase::Restoring:
        {
            _PendingRestoreReport = DoRun_Restore();
            ck::snapshot::Display(TEXT("DIAG: restored [{}] entities into post-travel world — stamping respawn markers"),
                _PendingRestoreReport.Get_EntitiesRestored());

            DoStamp_RespawnMarkers();

            // Run_Restore did registry.clear() + adopted a new transient. Every processor in this world cached its
            // registry/transient context at construction (pre-restore) and is now blind to the restored entities.
            // Rebuild the processor graph so processors are re-created against the restored registry — this is what
            // lets FProcessor_ActorRespawn (and any other processor) see the restored world. The rebuild is deferred
            // to OnEndFrame and is safe to call mid-tick.
            if (const auto World = GetWorld();
                ck::IsValid(World))
            {
                if (auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
                    ck::IsValid(EcsWorld))
                {
                    ck::snapshot::Display(TEXT("DIAG: rebuilding processor graph against the restored registry"));
                    EcsWorld->Request_RebuildProcessorGraph();
                }
            }

            _LoadFrameCount = 0; // reset so the respawn-drain cap measures from here
            _LoadPhase = ELoadPhase::RespawningActors;
            return true;
        }

        case ELoadPhase::RespawningActors:
        {
            // Stage 1: wait for FProcessor_ActorRespawn (scheduler tick) to spawn + re-bridge every stamped entity.
            if (NOT DoIs_RespawnComplete())
            {
                if (_LoadFrameCount >= kLoad_RespawnFrameCap)
                {
                    ck::snapshot::Error(TEXT("Request_Load: actor respawn did not drain within [{}] frames — aborting"),
                        kLoad_RespawnFrameCap);
                    auto Report = FCk_Snapshot_LoadReport{};
                    Report.Set_Result(ECk_SnapshotResult::Failed_IO);
                    DoFinish_Load(Report);
                    return false;
                }
                return true;
            }

            // Stage 2: let deferred WithActor constructs (from the respawned actors' BeginPlay) run + abstain while
            // the reconstitution flag is still set, before clearing it. The sentinel arms the countdown exactly
            // once — the natural terminal 0 must NOT re-arm it (that would loop forever).
            if (NOT _RespawnQuiescenceStarted)
            {
                _RespawnQuiescenceStarted = true;
                _RespawnQuiescenceFramesRemaining = kLoad_RespawnQuiescenceFrames;
            }
            if (_RespawnQuiescenceFramesRemaining-- > 0)
            { return true; }

            DoSet_ReconstitutionFlag(false);
            ck::snapshot::Display(TEXT("DIAG: respawn drained + quiescence complete — finishing load"));
            DoFinish_Load(_PendingRestoreReport);
            return false; // done — unregister
        }

        default:
            return false; // Idle / unexpected — stop ticking
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoFinish_Load(const FCk_Snapshot_LoadReport& InReport)
    -> void
{
    DoSet_ReconstitutionFlag(false); // defensive: a teardown/travel abort must never leave a world reconstituting
    _LoadTickerHandle.Reset(); // DoTick_Load returns false to unregister; just drop our copy of the handle
    _LoadPhase = ELoadPhase::Idle;
    _LoadInProgress = false;
    _PendingTeardownRoots.Reset();
    _PendingLoadBytes.Reset();

    const auto Delegate = _PendingLoadDelegate;
    _PendingLoadDelegate.Unbind();

    const auto Source = DoGet_SnapshotSource(); // re-resolve: the restored/adopted transient
    ck::UUtils_Signal_Snapshot_OnLoadComplete::Broadcast(Source, ck::MakePayload(Source, InReport));
    Delegate.ExecuteIfBound(InReport);
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Snapshot_Subsystem_UE::
    Get_HasSaveSlot(
        FName InSlotName) const
{
    return UGameplayStatics::DoesSaveGameExist(InSlotName.ToString(), ck_snapshot_subsystem::UserIndex);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Snapshot_Header
    UCk_Snapshot_Subsystem_UE::
    Get_SaveSlotHeader(
        FName InSlotName) const
{
    auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(UGameplayStatics::LoadGameFromSlot(InSlotName.ToString(), ck_snapshot_subsystem::UserIndex));
    if (ck::Is_NOT_Valid(SaveGame))
    { return {}; }

    return SaveGame->_Header;
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Snapshot_Subsystem_UE::
    TryResolve_SaveKey(
        FGuid InKey,
        FCk_Handle& OutHandle) const
{
    if (const auto* Found = _SaveKeyResolverMap.Find(InKey))
    {
        OutHandle = *Found;
        return ck::IsValid(OutHandle);
    }

    OutHandle = FCk_Handle{};
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    Publish_SaveKey(
        FGuid InKey,
        FCk_Handle InHandle)
    -> void
{
    _SaveKeyResolverMap.Add(InKey, InHandle);
}

auto
    UCk_Snapshot_Subsystem_UE::
    Consume_SaveKey(
        FGuid InKey)
    -> void
{
    _SaveKeyResolverMap.Remove(InKey);
}

// --------------------------------------------------------------------------------------------------------------------
