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
#include "CkEcsExt/OwningActor/CkActorRebind_Utils.h"         // M2b: Request_RebindActor
#include "CkEcsExt/Transform/CkTransform_Utils.h"             // M2b position-restore: restored spawn transform

#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"

#include <Engine/World.h>
#include <GameFramework/Actor.h>   // M2b: SpawnActor<AActor> needs the complete type
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

    ck::snapshot::Display(TEXT("DIAG: Request_Load travel — OpenLevel to map [{}] (pre-travel world [{}])"),
        _TravelMapName, World->GetName());

    constexpr auto AbsoluteTravel = true;
    UGameplayStatics::OpenLevel(World, FName{*_TravelMapName}, AbsoluteTravel);
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

    // A different world object than the one we left (OpenLevel produces a fresh world — Phase A confirmed it is
    // NM_Standalone, so detect by instance + HasBegunPlay, NOT netmode), and it has begun play (its EcsWorld
    // subsystem has re-inited a clean registry).
    if (World == _PreTravelWorld.Get())
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
    DoRespawn_BridgedActors()
    -> int32
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return 0; }

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return 0; }

    auto* RawRegistry = ck::registry_table::TryResolve(EcsWorld->Get_Registry().Get_RegistryHandle());
    if (RawRegistry == nullptr)
    { return 0; }

    const auto Transient = EcsWorld->Get_TransientEntity();

    // Collect the restored bridged entities FIRST — spawning actors mutates the registry, so do not spawn mid-view.
    // The raw entt view yields FCk_Entity::IdType; wrap in FCk_Entity for MakeHandle (per CkSnapshot_Context).
    auto BridgedEntities = TArray<FCk_Handle>{};
    for (const auto Entity : RawRegistry->view<FFragment_ActorSpawnIntent>())
    {
        BridgedEntities.Add(ck::MakeHandle(FCk_Entity{Entity}, Transient));
    }

    auto RespawnedCount = 0;
    for (auto& Entity : BridgedEntities)
    {
        if (ck::Is_NOT_Valid(Entity) || NOT Entity.Has<FFragment_ActorSpawnIntent>())
        { continue; }

        const auto& ActorClassPath = Entity.Get<FFragment_ActorSpawnIntent>().Get_ActorClassPath();
        auto* ActorClass = FSoftClassPath{ActorClassPath}.TryLoadClass<AActor>();
        if (ActorClass == nullptr)
        {
            ck::snapshot::Warning(TEXT("DIAG: respawn — entity [{}] actor class path [{}] unloadable; skipped"),
                Entity, ActorClassPath);
            continue;
        }
        ck::snapshot::Display(TEXT("DIAG: respawn — entity [{}] spawning actor of class [{}]"), Entity, ActorClassPath);

        // Spawn at the restored world transform (FFragment_Transform round-trips), so the actor starts at its
        // saved position; Request_RebindActor then re-binds its root component to the restored Transform value.
        const auto SpawnTransform = UCk_Utils_Transform_UE::Has(Entity)
            ? UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Entity)
            : FTransform::Identity;

        auto SpawnInfo = FActorSpawnParameters{};
        SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnInfo);
        if (Actor == nullptr)
        {
            ck::snapshot::Warning(TEXT("DIAG: respawn — SpawnActor failed for entity [{}]"), Entity);
            continue;
        }

        UCk_Utils_ActorRebind_UE::Request_RebindActor(Entity, Actor);
        ++RespawnedCount;
    }

    ck::snapshot::Display(TEXT("DIAG: respawn — re-spawned + re-bridged [{}] bridged actors"), RespawnedCount);
    return RespawnedCount;
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
            ck::snapshot::Display(TEXT("DIAG: restored [{}] entities into post-travel world — respawning actors"),
                _PendingRestoreReport.Get_EntitiesRestored());

            DoRespawn_BridgedActors();
            _RespawnQuiescenceFramesRemaining = kLoad_RespawnQuiescenceFrames;
            _LoadPhase = ELoadPhase::RespawningActors;
            return true;
        }

        case ELoadPhase::RespawningActors:
        {
            // Let deferred WithActor constructs (from respawned actors) run + abstain while the flag is still set.
            if (_RespawnQuiescenceFramesRemaining-- > 0)
            { return true; }

            DoSet_ReconstitutionFlag(false);
            ck::snapshot::Display(TEXT("DIAG: respawn quiescence complete — finishing load"));
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
