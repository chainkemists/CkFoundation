#include "CkSnapshot_Subsystem.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/Inspection/CkSnapshot_Inspection.h" // shared identity/provenance rendering + the DumpSlot census
#include "CkSnapshot/SaveGame/CkSnapshot_SaveGame.h"
#include "CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h" // v3 recipe+payload capture (the live save path)
#include "CkSnapshot/Subsystem/CkSnapshot_LoadState.h" // the ready-to-resume fact clients read
#include "CkSnapshot/Subsystem/CkSnapshot_Signals.h"

#include "CkActorRelay/CkActorRelay_Utils.h" // Request_AcquireChannel — the fact's net-correlated carrier

#include "CkLoadingScreen/LoadingProcess/CkLoadingProcess_Task.h" // the screen the whole load holds up

#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"                // EngineOwned rendezvous resolver
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"   // FFragment_LifetimeDependents, FTag_ConstructSpawned
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"           // FTag_Snapshot_SaveTransient (reconcile skip)
#include "CkEcs/Handle/CkHandle.h"                            // ck::FTag_DestroyEntity_*
#include "CkEcs/Handle/CkHandle_Utils.h"                      // ck::MakeHandle
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/EntityScript/CkEntityScript.h"                // UCk_EntityScript_UE
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"          // Request_SpawnEntity
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h" // Request_BuildAndReplicate (DefinitionBuilt)
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"          // Get_ContextOwner (DefinitionBuilt rebuild owner)
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"            // TryGet_ActorEntityHandle (bridged rendezvous)
#include "CkEcs/Snapshot/CkSnapshot_Context.h"                // ck::FSnapshotContext (v3 map-backed mode)
#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"             // ck::snapshot::RemapHandles
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h" // Get_SaveHandlerTypes/Resolve (reconcile payload probe)
#include "CkEcs/Persistence/CkPersistenceHydration.h" // FFragment_PendingHydration, FTag_Hydration_PendingApply (split Phase 5)
#include "CkEcs/Persistence/CkLoadConvergence_Registry.h" // the convergence phase's predicates + advances
#include "CkEcs/Net/CkNet_Utils.h" // TryAddContainerFragment — the ready-to-resume fact's wire path
#include "CkEcs/Tag/CkTag_HydrationQuarantine.h" // FTag_Hydration_Quarantine, FCtx_HydrationQuarantine

#include "CkEcsExt/Transform/CkTransform_Utils.h"             // G1 saved-world-transform restore (actor + pure-ECS)
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"     // FCk_Request_Transform_SetTransform (pure-ECS mover)

#include "CkCore/Algorithms/CkAlgorithms.h"                  // ck::algo::NoneOf
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"          // ResolveGameplayTag — the ActorRelay group tag
#include "CkCore/Time/CkTime_Utils.h"                        // Get_Milliseconds for the save-stage breakdown

#include "CkLabel/CkLabel_Utils.h"                            // ConstructSpawned adopt/reconcile by label

#include "HAL/IConsoleManager.h"    // Ck.Snapshot.DumpSlot census command
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "PlatformFeatures.h"       // ISaveGameSystem::GetSaveGameNames — slot enumeration
#include "SaveGameSystem.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include <Engine/NetDriver.h>   // UNetDriver::ClientConnections to decide seamless-vs-OpenLevel
#include <Engine/World.h>
#include "EngineUtils.h"         // TActorRange (player rendezvous)
#include <GameFramework/Actor.h>   // SpawnActor<AActor> needs the complete type
#include <GameFramework/GameModeBase.h>   // AGameModeBase::bUseSeamlessTravel for seamless ServerTravel
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/WorldSettings.h>   // AWorldSettings::TimeDilation — the load's game-time freeze
#include "UObject/SoftObjectPath.h" // FSoftClassPath::TryLoadClass for recipe/actor classes

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_subsystem
{
    constexpr auto UserIndex = 0;
    constexpr auto k_NoEntity = 0xFFFFFFFFu; // mirrors ck::snapshot's k_NoEntity sentinel

    // The post-condition the freeze has to actually achieve, not the value it asks for. SetTimeDilation clamps
    // its argument into [MinGlobalTimeDilation, MaxGlobalTimeDilation], so asking for the floor always
    // "succeeds" and returns whatever that floor happens to be — and the floor is EditAnywhere per level, saved
    // into the .umap. A map authored with a floor of 1.0 would take the write, log nothing, and ship with the
    // whole guarantee silently absent. 1% of real time is the loosest thing still worth calling frozen: the
    // engine's own default floor is 0.0001, four orders of magnitude under it.
    constexpr auto k_MaxFrozenTimeDilation = 0.01f;

    // Replays the capture's ArIsSaveGame tagged-property blob onto a deferred-spawn actor. Empty bytes mean the saved
    // class declared no SaveGame property (or the row predates the field), and the spawn proceeds untouched.
    auto
        DoApply_ActorSaveFields(
            AActor* InActor,
            const TArray<uint8>& InFieldBytes)
        -> void
    {
        if (InActor == nullptr || InFieldBytes.IsEmpty())
        { return; }

        auto Reader = FMemoryReader{InFieldBytes, /*bIsPersistent=*/true};
        constexpr auto LoadIfFindFails = true;
        auto Proxy = FObjectAndNameAsStringProxyArchive{Reader, LoadIfFindFails};
        Proxy.ArIsSaveGame = true;      // restore ONLY the CPF_SaveGame properties, symmetric with the capture
        Proxy.SetIsPersistent(true);

        InActor->SerializeScriptProperties(Proxy);
    }

    auto
        DoGet_HasWorldAuthority(
            const UWorld* InWorld)
        -> bool
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return false; }

        return InWorld->GetNetMode() != ENetMode::NM_Client;
    }

    // Mirrors the capture-side TryResolve_PlayerRendezvous identity; invalid handle when unresolved.
    auto
        DoResolve_PlayerEntity(
            UWorld* InWorld,
            const FString& InPlayerId)
        -> FCk_Handle
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return {}; }

        const auto TryPawnEntity = [](const APlayerState* InState) -> FCk_Handle
        {
            const auto* Pawn = InState != nullptr ? InState->GetPawn() : nullptr;
            if (Pawn == nullptr)
            { return {}; }
            return UCk_Utils_OwningActor_UE::TryGet_ActorEntityHandle(Pawn);
        };

        for (const auto* State : TActorRange<APlayerState>{InWorld})
        {
            if (State == nullptr)
            { continue; }

            const auto UniqueId = State->GetUniqueId();
            const auto IdString = UniqueId.IsValid() ? UniqueId.ToString() : FString{};

            // Empty saved id ⇒ standalone player 0: match the first player state.
            if (InPlayerId.IsEmpty() || IdString == InPlayerId)
            {
                if (auto Entity = TryPawnEntity(State); ck::IsValid(Entity))
                {
                    // A keyed live entity belongs to KEYED rows — the bridged pawn row adopts it. Legacy
                    // player-id rows (old saves' controller/state entities) must not pre-claim it, or the
                    // pawn's own row skips as DuplicateSaveKey and its whole subtree orphans.
                    if (Entity.Has<FFragment_SaveKey>())
                    { return {}; }
                    return Entity;
                }
            }
        }
        return {};
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

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_LoadWorldEcs() const
    -> UCk_EcsWorld_Subsystem_UE*
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    return World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Snapshot_Subsystem_UE::
    Get_IsLoadInProgress() const
{
    return _LoadInProgress;
}

bool
    UCk_Snapshot_Subsystem_UE::
    Get_IsReadyToResume() const
{
    return _IsReadyToResume;
}

auto
    UCk_Snapshot_Subsystem_UE::
    Get_LoadEpoch() const
    -> int32
{
    return _LoadEpoch;
}

bool
    UCk_Snapshot_Subsystem_UE::
    Get_IsSaveInProgress() const
{
    return _SnapshotInProgress;
}

FCk_Snapshot_LoadReport
    UCk_Snapshot_Subsystem_UE::
    Get_LastLoadReport() const
{
    return _LastLoadReport;
}

FCk_Snapshot_SaveReport
    UCk_Snapshot_Subsystem_UE::
    Get_LastSaveReport() const
{
    return _LastSaveReport;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (_LoadTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_LoadTickerHandle);
        _LoadTickerHandle.Reset();
    }

    // Tearing down mid-load must not leave anything the load put in place standing: a held world never ticks
    // again, a frozen world never runs again, and a held loading screen never comes down. Each is a permanent
    // wedge on its own, so this route releases all of them even though the world is going away anyway — the
    // GameInstance may outlive the world, and a leaked screen holder is outered to the GameInstance.
    DoSet_LoadHold(ECk_EcsWorld_LoadHold::None);

    if (auto* FrozenWorld = _TimeFreezeWorld.Get();
        FrozenWorld != nullptr)
    { DoRestore_TimeFreeze(*FrozenWorld); }

    DoRelease_LoadScreenHold(_LoadScreenHold);
    DoRelease_ClientHold(TEXT("the snapshot subsystem is being torn down"));

    _LoadInProgress = false;
    _IsReadyToResume = false;
    _LoadPhase = ELoadPhase::Idle;
    _RuntimeEntityScriptsAwaitingConstruction.Reset();
    _LoadStateChannelEntity = FCk_Handle{};
    _PendingLoadStateChannel = FCk_Handle_PendingActorRelay{};

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    Request_Save(
        FName InSlotName,
        const FCk_Delegate_OnSaveComplete& InDelegate)
{
    constexpr auto WriteSidecar = false;
    DoRequest_Save(InSlotName, FCk_Snapshot_SaveMetadata{}, WriteSidecar, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    Request_Save_WithMetadata(
        FName InSlotName,
        const FCk_Snapshot_SaveMetadata& InMetadata,
        const FCk_Delegate_OnSaveComplete& InDelegate)
{
    constexpr auto WriteSidecar = true;
    DoRequest_Save(InSlotName, InMetadata, WriteSidecar, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    Request_Save_WithFreshThumbnail(
        FName InSlotName,
        const FCk_Snapshot_SaveMetadata& InMetadata,
        const FCk_Delegate_OnSaveComplete& InDelegate,
        int32 InThumbnailMaxWidth)
{
    if (NOT InMetadata.Get_ScreenshotPng().IsEmpty())
    {
        Request_Save_WithMetadata(InSlotName, InMetadata, InDelegate);
        return;
    }

    // Weak self-capture: the capture resolves a frame later, and the subsystem can be torn down by a
    // travel in between. The completion always runs, so a dead subsystem must drop the save rather
    // than write into a destroyed world.
    ck::snapshot::slot_meta::Request_CaptureViewportPng(GetWorld(), InThumbnailMaxWidth,
        [WeakSelf = TWeakObjectPtr<UCk_Snapshot_Subsystem_UE>{this}, InSlotName, InMetadata, InDelegate]
        (TArray<uint8> InPng) -> void
        {
            auto* Self = WeakSelf.Get();

            if (ck::Is_NOT_Valid(Self))
            {
                ck::snapshot::Warning(TEXT("Request_Save_WithFreshThumbnail: the snapshot subsystem went away while the "
                    "thumbnail was in flight — slot [{}] was NOT saved."), InSlotName);
                InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_NotQuiescent);
                return;
            }

            // An empty capture is not a save failure — the slot simply gets no picture.
            auto Metadata = InMetadata;
            Metadata.Set_ScreenshotPng(InPng);

            Self->Request_Save_WithMetadata(InSlotName, Metadata, InDelegate);
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRequest_Save(
        FName InSlotName,
        const FCk_Snapshot_SaveMetadata& InMetadata,
        bool InWriteSidecar,
        const FCk_Delegate_OnSaveComplete& InDelegate)
    -> void
{
    const auto World = GetWorld();

    const auto HasAuthorityWorld = ck::IsValid(World) && ck_snapshot_subsystem::DoGet_HasWorldAuthority(World);
    CK_ENSURE_IF_NOT(HasAuthorityWorld,
        TEXT("Request_Save refused: World [{}] is invalid or this is a client (no authority)"), World)
    {
        InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_IO);
        return;
    }

    const auto CanStartSnapshot = NOT _SnapshotInProgress && NOT _LoadInProgress;
    CK_ENSURE_IF_NOT(CanStartSnapshot,
        TEXT("Request_Save refused: a snapshot operation is already in progress"))
    {
        InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_IO);
        return;
    }

    _SnapshotInProgress = true;
    ON_SCOPE_EXIT { _SnapshotInProgress = false; };

    // A save is one synchronous game-thread frame, so the stage breakdown below IS the hitch profile. Logged
    // unconditionally because it has to be readable from a packaged build with no Insights trace attached.
    TRACE_CPUPROFILER_EVENT_SCOPE(UCk_Snapshot_Subsystem_UE_DoRequest_Save);

    auto PumpTime      = FCk_Time{};
    auto SerializeTime = FCk_Time{};
    auto IoTime        = FCk_Time{};
    auto SidecarTime   = FCk_Time{};

    auto Source = DoGet_SnapshotSource();
    ck::UUtils_Signal_Snapshot_OnPreSave::Broadcast(Source, ck::MakePayload(Source));

    if (auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        ck::IsValid(EcsWorld))
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(CkSnapshot_Save_PumpToQuiescence);
        const auto PumpStopwatch = FCk_ScopedStopwatch{PumpTime};
        _LastPumpCount = EcsWorld->Request_PumpToQuiescence();
        ck::snapshot::Verbose(TEXT("Request_Save: pumped world to quiescence in [{}] pump passes"), _LastPumpCount);
    }

    auto ByteWriterV3 = FBufferArchive{};
    auto HeaderV3 = FCk_Snapshot_HeaderV3{};
    _LastSaveReport = FCk_Snapshot_SaveReport{};
    auto CaptureTimings = ck::snapshot::FCaptureTimings{};
    const auto CaptureResultV3 = ck::snapshot::Run_CaptureV3(*World, ByteWriterV3, HeaderV3,
        _LastSaveReport, &CaptureTimings);
    _LastSaveReport.Set_Result(CaptureResultV3);

    auto DoFinish = [&](ECk_SnapshotResult InResult) -> void
    {
        ck::UUtils_Signal_Snapshot_OnSaveComplete::Broadcast(Source, ck::MakePayload(Source, InResult));
        InDelegate.ExecuteIfBound(InResult);
    };

    if (CaptureResultV3 != ECk_SnapshotResult::Success)
    {
        ck::snapshot::Error(TEXT("Request_Save: v3 capture failed with result [{}]"), CaptureResultV3);
        DoFinish(CaptureResultV3);
        return;
    }

    auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SaveGame::StaticClass()));
    const auto HasSaveGame = ck::IsValid(SaveGame);
    CK_ENSURE_IF_NOT(HasSaveGame,
        TEXT("Request_Save: failed to create UCk_Snapshot_SaveGame"))
    {
        DoFinish(ECk_SnapshotResult::Failed_IO);
        return;
    }

    SaveGame->_HeaderV3 = HeaderV3;
    SaveGame->_SnapshotBytesV3 = MoveTemp(static_cast<TArray<uint8>&>(ByteWriterV3));
    const auto SavedByteCount = SaveGame->_SnapshotBytesV3.Num();

    // The two halves of SaveGameToSlot, kept apart because only the io leg could ever move off the game
    // thread — AsyncSaveGameToSlot runs SaveGameToMemory inline (GameplayStatics.cpp:2403).
    auto ObjectBytes = TArray<uint8>{};

    const auto Serialized = [&]() -> bool
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(CkSnapshot_Save_SaveGameToMemory);
        const auto SerializeStopwatch = FCk_ScopedStopwatch{SerializeTime};
        return UGameplayStatics::SaveGameToMemory(SaveGame, ObjectBytes);
    }();

    const auto Saved = Serialized && [&]() -> bool
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(CkSnapshot_Save_SaveDataToSlot);
        const auto IoStopwatch = FCk_ScopedStopwatch{IoTime};
        return UGameplayStatics::SaveDataToSlot(ObjectBytes, InSlotName.ToString(), ck_snapshot_subsystem::UserIndex);
    }();

    if (NOT Saved)
    {
        ck::snapshot::Error(TEXT("Request_Save: [{}] failed for slot [{}]"),
            Serialized ? TEXT("SaveDataToSlot") : TEXT("SaveGameToMemory"), InSlotName);
        DoFinish(ECk_SnapshotResult::Failed_IO);
        return;
    }

    if (InWriteSidecar)
    {
        const auto SidecarStopwatch = FCk_ScopedStopwatch{SidecarTime};
        DoWrite_SlotMeta(InSlotName, InMetadata, HeaderV3);
    }

    ck::snapshot::Display(TEXT("Request_Save: saved [{}] v3 bytes to slot [{}]"), SavedByteCount, InSlotName);

    const auto CaptureTime = CaptureTimings.Classify + CaptureTimings.Payloads + CaptureTimings.Tables;
    const auto WriteTime   = SerializeTime + IoTime;

    ck::snapshot::Display(
        TEXT("Request_Save TIMING slot [{}]: total [{:.2f}ms] = pump [{:.2f}ms] ([{}] passes) + capture [{:.2f}ms] "
             "(classify [{:.2f}ms], payloads [{:.2f}ms] (produce [{:.2f}ms], serialize [{:.2f}ms]), "
             "tables [{:.2f}ms]) + write [{:.2f}ms] "
             "(serialize [{:.2f}ms], io [{:.2f}ms]) + sidecar [{:.2f}ms]. "
             "Audit [{:.2f}ms] over [{}] probes (inside classify). [{}] entities, [{}] payloads ([{}] distinct types), "
             "[{}] bytes ([{}] payload + [{}] structural)."),
        InSlotName,
        UCk_Utils_Time_UE::Get_Milliseconds(PumpTime + CaptureTime + WriteTime + SidecarTime),
        UCk_Utils_Time_UE::Get_Milliseconds(PumpTime),
        _LastPumpCount,
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTime),
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTimings.Classify),
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTimings.Payloads),
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTimings.PayloadsProduce),
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTimings.PayloadsSerialize),
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTimings.Tables),
        UCk_Utils_Time_UE::Get_Milliseconds(WriteTime),
        UCk_Utils_Time_UE::Get_Milliseconds(SerializeTime),
        UCk_Utils_Time_UE::Get_Milliseconds(IoTime),
        UCk_Utils_Time_UE::Get_Milliseconds(SidecarTime),
        UCk_Utils_Time_UE::Get_Milliseconds(CaptureTimings.Audit),
        CaptureTimings.AuditProbeCount,
        HeaderV3.Get_EntityCount(), HeaderV3.Get_PayloadCount(), CaptureTimings.DistinctTypePaths,
        SavedByteCount, CaptureTimings.PayloadByteTotal, SavedByteCount - CaptureTimings.PayloadByteTotal);

    DoFinish(ECk_SnapshotResult::Success);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoWrite_SlotMeta(
        FName InSlotName,
        const FCk_Snapshot_SaveMetadata& InMetadata,
        const FCk_Snapshot_HeaderV3& InHeader) const
    -> void
{
    auto* MetaSaveGame = Cast<UCk_Snapshot_SlotMetaSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UCk_Snapshot_SlotMetaSaveGame::StaticClass()));

    if (ck::Is_NOT_Valid(MetaSaveGame))
    {
        ck::snapshot::Error(TEXT("DoWrite_SlotMeta: failed to create the sidecar SaveGame for slot [{}]"), InSlotName);
        return;
    }

    // The thumbnail is supplied by the caller or absent — there is no capture here. A viewport read is
    // frame-deferred, and this save is synchronous; see FCk_Snapshot_SaveMetadata::_ScreenshotPng.
    MetaSaveGame->_Meta.Set_SlotName(InSlotName)
                       .Set_Title(InMetadata.Get_Title())
                       .Set_TimestampUTC(InHeader.Get_TimestampUTC())
                       .Set_WorldAssetPath(InHeader.Get_WorldAssetPath())
                       .Set_ScreenshotPng(InMetadata.Get_ScreenshotPng())
                       .Set_CustomFields(InMetadata.Get_CustomFields());

    // A sidecar failure must not fail the save — the snapshot is already on disk and is the thing
    // that matters; the slot simply lists untitled until it is saved over.
    if (NOT UGameplayStatics::SaveGameToSlot(MetaSaveGame,
            ck::snapshot::slot_meta::Get_MetaSlotName(InSlotName), ck_snapshot_subsystem::UserIndex))
    { ck::snapshot::Error(TEXT("DoWrite_SlotMeta: SaveGameToSlot failed for the sidecar of slot [{}]"), InSlotName); }
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

    const auto HasAuthorityWorld = ck::IsValid(World) && ck_snapshot_subsystem::DoGet_HasWorldAuthority(World);
    CK_ENSURE_IF_NOT(HasAuthorityWorld,
        TEXT("Request_Load refused: World [{}] is invalid or this is a client (no authority)"), World)
    {
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IO));
        return;
    }

    const auto CanStartSnapshot = NOT _SnapshotInProgress && NOT _LoadInProgress;
    CK_ENSURE_IF_NOT(CanStartSnapshot,
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

    if (SaveGame->_HeaderV3.Get_FormatVersion() != FCk_Snapshot_HeaderV3::CurrentFormatVersion ||
        SaveGame->_SnapshotBytesV3.IsEmpty())
    {
        ck::snapshot::Error(TEXT("Request_Load: slot [{}] has no compatible v3 payload (v3 version [{}], [{}] bytes) — "
            "rebuild+hydrate requires a v3 save"), InSlotName,
            SaveGame->_HeaderV3.Get_FormatVersion(), SaveGame->_SnapshotBytesV3.Num());
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IncompatibleSave));
        return;
    }

    // Before teardown — a corrupt stream must abort while the world is still alive.
    _V3Tables = FCk_Snapshot_V3_Tables{};
    _V3Header = SaveGame->_HeaderV3;
    {
        auto Reader = FMemoryReader{SaveGame->_SnapshotBytesV3, /*bIsPersistent=*/true};
        FCk_Snapshot_V3_Tables::StaticStruct()->SerializeItem(Reader, &_V3Tables, /*Defaults=*/nullptr);
        if (Reader.IsError())
        {
            ck::snapshot::Error(TEXT("Request_Load: v3 stream in slot [{}] is corrupt (deserialize failed)"), InSlotName);
            InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_Corrupt));
            return;
        }
    }

    // ---- Latch the load (spans real frames + a level reload from here) -------------------------------------
    _LoadInProgress      = true;
    _IsReadyToResume     = false;
    _LoadPhase           = ELoadPhase::TearingDown;
    _PendingLoadDelegate = InDelegate;
    _LoadFrameCount      = 0;
    _PreTravelWorld      = nullptr;
    _TravelMapName.Reset();

    // Counted per GameInstance, and never reused: it rides the travel URL and the replicated fact, and it is
    // what stops a client that travelled for THIS load releasing on a fact the previous one left standing.
    ++_LoadEpoch;
    _ConvergenceFramesSatisfied = 0;
    _LoadStateChannelEntity = FCk_Handle{};
    _PendingLoadStateChannel = FCk_Handle_PendingActorRelay{};

    _SavedIdMap.Reset();
    _MappedLiveEntities.Reset();
    _SpawnedRuntimeIds.Reset();
    _RuntimeEntityScriptsAwaitingConstruction.Reset();
    _SkippedIds.Reset();
    _SkipRecords.Reset();
    _PersistedIds.Reset();
    for (const auto& Entry : _V3Tables.Get_Entities())
    { _PersistedIds.Add(Entry.Get_SavedId()); }
    _PendingBridgeActors.Reset();
    _V3LoadReport = FCk_Snapshot_LoadReport{};
    _V3LoadReport.Set_Result(ECk_SnapshotResult::Success);
    _HydrationEnqueued = false;
    _QuarantineStamped = false;
    _QuarantineLifted  = false;
    _SettleFramesRemaining = 0;
    _SettleStarted = false;
    _RebuildLastMappedCount = 0;
    _RebuildStallTicks = 0;
    _RebuildEscalated = false;

    {
        auto EngineOwned = 0, ConstructSpawned = 0, RuntimeSpawned = 0, Bridged = 0;
        for (const auto& Entry : _V3Tables.Get_Entities())
        {
            switch (Entry.Get_Provenance())
            {
                case ECk_Snapshot_V3_Provenance::EngineOwned:      ++EngineOwned; break;
                case ECk_Snapshot_V3_Provenance::ConstructSpawned: ++ConstructSpawned; break;
                case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                    ++RuntimeSpawned;
                    if (NOT Entry.Get_ActorClassPath().IsEmpty())
                    { ++Bridged; }
                    break;
            }
        }
        ck::snapshot::Display(TEXT("DIAG: v3 table provenance — EngineOwned [{}], ConstructSpawned [{}], RuntimeSpawned [{}] (of which bridged [{}])"),
            EngineOwned, ConstructSpawned, RuntimeSpawned, Bridged);
    }

    // The screen goes up BEFORE the first entity is destroyed, and stays up until the world is ready to resume.
    // Everything between those two points — the teardown cascade, the travel, the rebuild, the drain, the
    // convergence — is a world the player has no business watching.
    DoCreate_LoadScreenHold(World, TEXT("CkSnapshot: load in progress (not yet ready to resume)"), _LoadScreenHold);

    // The world about to be demolished is frozen too. Its destruction cascade must complete, and the 136 EndPlay
    // processors that run it are outside the load kernel — but nothing in it should be PACED while it happens.
    // The freeze does not survive the travel; the post-travel world is frozen again at its boot seed.
    DoApply_TimeFreeze(*World);
    DoSet_LoadHold(ECk_EcsWorld_LoadHold::Teardown);

    // Acquired now, on the pre-travel world, so the fact has somewhere to land the moment the load reaches it.
    // Pooled and shared with live consumers — acquire-only by design, so there is nothing to release.
    DoAcquire_LoadStateChannel(*World, false);
    DoPublish_LoadState(false);

    auto Source = DoGet_SnapshotSource();
    ck::UUtils_Signal_Snapshot_OnPreLoad::Broadcast(Source, ck::MakePayload(Source));

    DoInitiate_Teardown();

    _LoadTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_Snapshot_Subsystem_UE::DoTick_Load));

    ck::snapshot::Display(TEXT("Request_Load: v3 load [epoch {}] started for slot [{}] ([{}] entities, [{}] payloads; [{}] roots tearing down)"),
        _LoadEpoch, InSlotName, _V3Tables.Get_Entities().Num(), _V3Tables.Get_Payloads().Num(), _PendingTeardownRoots.Num());
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
    // A root's handle goes invalid only AFTER its whole subtree's cascade (incl. EndPlay) ran, so this covers children.
    return ck::algo::NoneOf(_PendingTeardownRoots,
        [](const auto& InRoot) { return ck::IsValid(InRoot); });
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

    // Travel to the world the SNAPSHOT was captured in, not the one we happen to be standing in.
    // Re-opening the current map is only correct for the save-then-load-in-place case; loading from
    // a frontend map (the main menu's Load button) would otherwise re-open the MAIN MENU and then
    // rebuild a gameplay world into it, and cross-map saves would restore into the wrong level.
    // The current world stays the fallback for a save with no recorded path (pre-v4 rows).
    const auto CurrentMapName = World->RemovePIEPrefix(World->GetOutermost()->GetName());
    const auto& SavedWorldPath = _V3Header.Get_WorldAssetPath();

    _TravelMapName = SavedWorldPath.IsValid()
        ? World->RemovePIEPrefix(SavedWorldPath.GetLongPackageName())
        : CurrentMapName;

    if (_TravelMapName.IsEmpty())
    { _TravelMapName = CurrentMapName; }

    if (_TravelMapName != CurrentMapName)
    {
        ck::snapshot::Display(TEXT("Request_Load travel — snapshot was captured in map [{}]; travelling there from [{}]"),
            _TravelMapName, CurrentMapName);
    }

    constexpr auto AbsoluteTravel = true;

    // The option rides BOTH travel shapes, because it is what tells a world coming up on the other side that a
    // load owns it — on this instance and, through ProcessClientTravel's round-trip of the same FURL, on every
    // client that follows. It is CkLoad and never "load": UWorld::SetupLevel skips assigning URL when the
    // incoming one carries an option by that exact name, which would silently make it unreadable at begin play.
    const auto LoadEpochOption = FString::Printf(TEXT("CkLoad=%d"), _LoadEpoch);

    // Seamless ServerTravel is connection-preserving but heavier — with no remote clients, OpenLevel is correct.
    auto* NetDriver = World->GetNetDriver();
    const auto HasConnectedClients =
        ck::IsValid(NetDriver) && NetDriver->ClientConnections.Num() > 0;

    if (World->GetNetMode() == ENetMode::NM_Standalone || NOT HasConnectedClients)
    {
        ck::snapshot::Display(TEXT("DIAG: Request_Load travel — OpenLevel (no connected clients) to map [{}] (pre-travel world [{}], epoch [{}])"),
            _TravelMapName, World->GetName(), _LoadEpoch);
        UGameplayStatics::OpenLevel(World, FName{*_TravelMapName}, AbsoluteTravel, LoadEpochOption);
        return;
    }

    if (auto* GameMode = World->GetAuthGameMode();
        ck::IsValid(GameMode))
    {
        GameMode->bUseSeamlessTravel = true;
    }
    else
    {
        ck::snapshot::Error(TEXT("DoInitiate_Travel: no authoritative GameMode on a server world — seamless travel "
            "NOT enabled; clients will NOT follow the reload. This should not happen on an authoritative server."));
    }

    ck::snapshot::Display(TEXT("DIAG: Request_Load travel — seamless ServerTravel (netmode [{}], [{}] client connection(s)) to map [{}] (pre-travel world [{}])"),
        static_cast<int32>(World->GetNetMode()), NetDriver->ClientConnections.Num(), _TravelMapName, World->GetName());

    World->ServerTravel(_TravelMapName + TEXT("?listen?") + LoadEpochOption, AbsoluteTravel);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_WorldOwnedByThisLoad(
        const UWorld& InWorld) const
    -> bool
{
    if (&InWorld == _PreTravelWorld.Get())
    { return false; }

    // Skip the seamless-travel transition map (different package name); the destination equals _TravelMapName.
    return InWorld.RemovePIEPrefix(InWorld.GetOutermost()->GetName()) == _TravelMapName;
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

    // Identity and READINESS are separate questions asked of the same rule: the boot seed has to answer the first
    // one at OnWorldBeginPlay, before HasBegunPlay is even true, and a second copy of the identity rule would be
    // free to drift from this one.
    return DoIs_WorldOwnedByThisLoad(*World) && World->HasBegunPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRehydrate_SaveKeyResolver()
    -> int32
{
    _SaveKeyResolverMap.Reset(); // pre-load entries point at pre-travel handles — dead after the world swap

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

    // The LIVE world-side SaveKey fragments (level actors the normal world build re-created), never restored ones.
    auto PublishedCount = 0;
    for (const auto Entity : RawRegistry->view<FFragment_SaveKey>())
    {
        const auto Handle = ck::MakeHandle(FCk_Entity{Entity}, CkRegistry);
        if (ck::Is_NOT_Valid(Handle))
        { continue; }

        const auto& Frag = RawRegistry->get<FFragment_SaveKey>(Entity);
        if (TryPublish_SaveKey(Frag.Get_Key(), Handle))
        { ++PublishedCount; }
        for (const auto& Alias : Frag.Get_Aliases())
        {
            if (TryPublish_SaveKey(Alias, Handle))
            { ++PublishedCount; }
        }
    }

    return PublishedCount;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoDeserialize_V3Blob(
        const TArray<uint8>& InBytes) const
    -> FInstancedStruct
{
    auto Out = FInstancedStruct{};
    if (InBytes.IsEmpty())
    { return Out; }

    auto Reader = FMemoryReader{InBytes, /*bIsPersistent=*/true};
    constexpr auto LoadIfFindFails = true;
    auto Proxy = FObjectAndNameAsStringProxyArchive{Reader, LoadIfFindFails};
    Proxy.ArIsSaveGame = false;      // symmetric with ck::snapshot::SerializeInstancedStruct
    Proxy.SetIsPersistent(true);

    const auto World = GetWorld();
    auto* EcsWorld = ck::IsValid(World) ? World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>() : nullptr;
    const auto LoadRegistryHandle = ck::IsValid(EcsWorld)
        ? EcsWorld->Get_Registry().Get_RegistryHandle()
        : FCk_RegistryHandle::Unset();
    auto Context = ck::FSnapshotContext{&_SavedIdMap, LoadRegistryHandle};

    Out.Serialize(Proxy);
    ck::snapshot::RemapHandles(Out.GetScriptStruct(), Out.GetMutableMemory(), Proxy, Context);
    return Out;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRebuild_Tick()
    -> bool
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return true; }

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return true; }

    const auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);

    // The world-ready sweep only sees keys that exist at BeginPlay. On-demand infrastructure (ActorRelay channels)
    // stamps its key ticks later, so an EngineOwned entry resolving against a stale map would orphan its whole
    // channel-owned subtree.
    DoRehydrate_SaveKeyResolver();

    auto AnyUnresolved = false;

    // The loader's own spawn window: recipe replays and definition rebuilds issued by this rebuild tick ARE
    // the world reconstitution and pass the load-gate spawn suppression. The window spans the kernel pump too
    // (the kernel scope carries no world-policy processors); the escalated full-scope passes run on the world
    // actor's own Tick, OUTSIDE this window — there, only construction cascades are admitted.
    const auto LoaderSpawnWindow = FCk_ScopedLoaderSpawnWindow{EcsWorld};

    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        const auto SavedId = Entry.Get_SavedId();
        if (_SavedIdMap.Contains(SavedId) || _SkippedIds.Contains(SavedId))
        { continue; } // already settled (mapped or deliberately skipped)

        auto Resolved = FCk_Handle{};

        switch (Entry.Get_Provenance())
        {
            case ECk_Snapshot_V3_Provenance::EngineOwned:
            {
                if (Entry.Get_SaveKey().IsValid())
                {
                    auto Found = FCk_Handle{};
                    if (TryResolve_SaveKey(Entry.Get_SaveKey(), Found))
                    { Resolved = Found; }
                }
                else
                {
                    Resolved = ck_snapshot_subsystem::DoResolve_PlayerEntity(World, Entry.Get_PlayerId());
                }
                break;
            }
            case ECk_Snapshot_V3_Provenance::ConstructSpawned:
            {
                // Adopt by (owner, label) — needs the owner mapped first (owners precede dependents in the table).
                // A child another row already claimed is excluded, so equal-label siblings (e.g. an attribute and
                // its refill child sharing one name) bind positionally: table order and LifetimeDependents order
                // are both creation order.
                const auto* Owner = _SavedIdMap.Find(Entry.Get_LifetimeOwnerSavedId());
                if (Owner != nullptr && ck::IsValid(*Owner) && Owner->Has<ck::FFragment_LifetimeDependents>())
                {
                    const auto& Label = Entry.Get_Label();
                    for (auto& Child : Owner->Get<ck::FFragment_LifetimeDependents>().Get_Entities())
                    {
                        if (ck::Is_NOT_Valid(Child) || NOT Child.Has<ck::FTag_ConstructSpawned>())
                        { continue; }
                        if (_MappedLiveEntities.Contains(Child))
                        { continue; }
                        if (NOT UCk_Utils_GameplayLabel_UE::Has(Child) || UCk_Utils_GameplayLabel_UE::Get_IsUnnamedLabel(Child))
                        { continue; }
                        if (UCk_Utils_GameplayLabel_UE::Get_Label(Child).ToString() == Label)
                        { Resolved = Child; break; }
                    }
                }
                break;
            }
            case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
            {
                const auto bBridged = NOT Entry.Get_ActorClassPath().IsEmpty();

                if (bBridged)
                {
                    // Rendezvous-first for KEYED bridged rows: when the retained SaveKey already resolves to a live
                    // entity, the fresh world created its own copy (a GameMode-spawned player pawn) — ADOPT it instead
                    // of respawning, or the impostor pair survives as an unpossessed statue that the NEXT save captures
                    // as one more row (the duplicate-player incident, 2026-07-28). A key that resolves to an entity
                    // another row already claimed is a second saved copy of the same logical entity — skip it, so
                    // distinct rows never consolidate. While the rebuild is still mapping entities elsewhere, an
                    // unresolved key WAITS (the key-holder may construct ticks later); once the rebuild quiesces with
                    // the key still unresolved (a DefaultPawn-GameMode map — no fresh copy exists), fall through to
                    // the actor respawn below, which is the loader-owned rebuild this row declared.
                    if (NOT _SpawnedRuntimeIds.Contains(SavedId) && Entry.Get_SaveKey().IsValid())
                    {
                        auto Found = FCk_Handle{};
                        if (TryResolve_SaveKey(Entry.Get_SaveKey(), Found) && ck::IsValid(Found))
                        {
                            if (_MappedLiveEntities.Contains(Found))
                            {
                                ck::snapshot::Warning(
                                    TEXT("v3 load SKIP: saved-id [{}] provenance [{}] identity [{}] owner [{}] reason [{}]"),
                                    SavedId, ck::snapshot::Get_ProvenanceText(Entry.Get_Provenance()),
                                    ck::snapshot::Get_IdentityText(Entry), Entry.Get_LifetimeOwnerSavedId(),
                                    ECk_Snapshot_SkipReason::DuplicateSaveKey);
                                DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::DuplicateSaveKey);
                                break;
                            }

                            // The respawn path seeds the actor spawn with the saved transform, and
                            // DoApply_SavedTransforms deliberately skips bridged rows — an ADOPTED actor got
                            // neither, so place it here. For a possessed pawn, also align the controller's
                            // control rotation: a Character's yaw follows it every tick, and the adopt path
                            // (unlike respawn-then-possess) inherits a STALE control rotation that would stomp
                            // the restored facing on the next move tick.
                            if (auto* AdoptedActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(Found);
                                AdoptedActor != nullptr)
                            {
                                constexpr auto Sweep = false;
                                AdoptedActor->SetActorTransform(Entry.Get_ActorSpawnTransform(), Sweep,
                                    nullptr, ETeleportType::TeleportPhysics);

                                if (const auto* AdoptedPawn = Cast<APawn>(AdoptedActor))
                                {
                                    if (auto* Controller = AdoptedPawn->GetController();
                                        Controller != nullptr)
                                    { Controller->SetControlRotation(Entry.Get_ActorSpawnTransform().Rotator()); }
                                }
                            }

                            Resolved = Found;
                            break;
                        }

                        // Fall through to the respawn only at FINAL quiesce: the fresh copy may be created by a
                        // game processor the kernel cannot run (a multi-stage construction), so the key can appear
                        // only under the ESCALATED full-scope ticks. Respawning any earlier re-opens the
                        // duplicate-statue class for late-keyed fresh copies.
                        if (NOT _RebuildEscalated || _RebuildStallTicks < 1)
                        {
                            AnyUnresolved = true; // key-holder may still be constructing — wait while others progress
                            break;
                        }
                    }

                    // Actor-first: spawn the actor at its saved transform; its own BeginPlay re-creates the
                    // WithActor entity (Construct composes features). Rendezvous-map via the actor once the bridge links.
                    if (NOT _SpawnedRuntimeIds.Contains(SavedId))
                    {
                        _SpawnedRuntimeIds.Add(SavedId);
                        auto* ActorClass = FSoftClassPath{Entry.Get_ActorClassPath()}.TryLoadClass<AActor>();
                        if (ActorClass == nullptr)
                        {
                            ck::snapshot::Error(TEXT("v3 load: RuntimeSpawned bridged entity [{}] actor class [{}] "
                                "unloadable — orphaned"), SavedId, Entry.Get_ActorClassPath());
                            DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::ClassUnloadable);
                        }
                        else
                        {
                            // Deferred: the saved SaveGame-flagged properties must land BEFORE BeginPlay, because the
                            // WithActor entity Construct it drives reads them (an item with a null definition composes
                            // no holder and no pickup probe). An entry with no bytes finishes spawning unchanged.
                            auto NoSpawnOwner = static_cast<AActor*>(nullptr);
                            auto NoSpawnInstigator = static_cast<APawn*>(nullptr);
                            auto* Actor = World->SpawnActorDeferred<AActor>(ActorClass, Entry.Get_ActorSpawnTransform(),
                                NoSpawnOwner, NoSpawnInstigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                            if (Actor != nullptr)
                            {
                                ck_snapshot_subsystem::DoApply_ActorSaveFields(Actor, Entry.Get_ActorSaveFieldBytes());

                                // The finish transform IS the one the deferred spawn already applied, so it is the
                                // default one — the same call the non-deferred SpawnActor made for us before.
                                constexpr auto IsDefaultTransform = true;
                                Actor->FinishSpawning(Entry.Get_ActorSpawnTransform(), IsDefaultTransform);
                            }

                            if (Actor == nullptr)
                            {
                                ck::snapshot::Error(TEXT("v3 load: SpawnActor failed for bridged entity [{}] class [{}]"),
                                    SavedId, Entry.Get_ActorClassPath());
                                DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::SpawnFailed);
                            }
                            else
                            { _PendingBridgeActors.Add(SavedId, Actor); }
                        }
                    }

                    if (const auto* Pending = _PendingBridgeActors.Find(SavedId);
                        Pending != nullptr && Pending->IsValid())
                    {
                        const auto Entity = UCk_Utils_OwningActor_UE::TryGet_ActorEntityHandle(Pending->Get());
                        if (ck::IsValid(Entity))
                        {
                            Resolved = Entity;
                            _PendingBridgeActors.Remove(SavedId);
                        }
                    }
                }
                else
                {
                    if (NOT _SpawnedRuntimeIds.Contains(SavedId))
                    {
                        auto* ScriptClass = FSoftClassPath{Entry.Get_ScriptClassPath()}.TryLoadClass<UCk_EntityScript_UE>();
                        if (ScriptClass == nullptr)
                        {
                            ck::snapshot::Error(TEXT("v3 load: RuntimeSpawned entity [{}] script class [{}] unloadable — orphaned"),
                                SavedId, Entry.Get_ScriptClassPath());
                            DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::ClassUnloadable);
                            break;
                        }

                        const auto* ScriptDefault = ScriptClass->GetDefaultObject<UCk_EntityScript_UE>();
                        const auto bSnapshotRespawnable =
                            ck::IsValid(ScriptDefault) && ScriptDefault->Get_IsSnapshotRespawnable();

                        const auto OwnerSavedId = Entry.Get_LifetimeOwnerSavedId();
                        auto Owner = TransientEntity;
                        if (OwnerSavedId != ck_snapshot_subsystem::k_NoEntity)
                        {
                            // Owner absent from the saved table ⇒ the world root/transient (never persisted). Opted-in
                            // entities respawn there; boot infra is skipped so the fresh world's copy is not duplicated.
                            if (NOT _PersistedIds.Contains(OwnerSavedId))
                            {
                                if (NOT bSnapshotRespawnable)
                                {
                                    ck::snapshot::Warning(
                                        TEXT("v3 load SKIP: saved-id [{}] provenance [{}] identity [{}] owner [{}] reason [{}]"),
                                        SavedId, ck::snapshot::Get_ProvenanceText(Entry.Get_Provenance()),
                                        ck::snapshot::Get_IdentityText(Entry), OwnerSavedId,
                                        ECk_Snapshot_SkipReason::NonPersistedOwnerNotRespawnable);
                                    DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::NonPersistedOwnerNotRespawnable);
                                    break;
                                }
                            }
                            else
                            {
                                const auto* MappedOwner = _SavedIdMap.Find(OwnerSavedId);
                                if (MappedOwner == nullptr || ck::Is_NOT_Valid(*MappedOwner))
                                {
                                    AnyUnresolved = true; // persisted owner not mapped yet — retry next tick
                                    break;
                                }
                                Owner = *MappedOwner;
                            }
                        }

                        _SpawnedRuntimeIds.Add(SavedId);
                        auto Params = DoDeserialize_V3Blob(Entry.Get_SpawnParamsBytes());
                        auto OnSpawnRequestCompleted = FCk_Delegate_Request_OnCompleted{};
                        OnSpawnRequestCompleted.BindDynamic(
                            this, &UCk_Snapshot_Subsystem_UE::DoOnRuntimeEntityScriptSpawnRequestCompleted);
                        auto Pending = UCk_Utils_EntityScript_UE::Request_SpawnEntity(
                            Owner, ScriptClass, Params, OnSpawnRequestCompleted);
                        // The pending handle wraps the immediately-created entity (Construct completes over the pumps);
                        // map it now so dependents can reference it.
                        Resolved = Pending.Get_EntityUnderConstruction();
                        if (ck::IsValid(Resolved))
                        {
                            _RuntimeEntityScriptsAwaitingConstruction.Add(Resolved);

                            auto OnConstructed = FCk_Delegate_EntityScript_Constructed{};
                            OnConstructed.BindDynamic(this, &UCk_Snapshot_Subsystem_UE::DoOnRuntimeEntityScriptConstructed);
                            UCk_Utils_PendingEntityScript_UE::Promise_OnConstructed(Pending, OnConstructed);
                        }
                    }
                }
                break;
            }
            case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
            {
                if (NOT _SpawnedRuntimeIds.Contains(SavedId))
                {
                    // Prefer the captured CONTEXT owner — the driver-bearing subject production built the item under
                    // (Create(Get_ContextOwner(inventory), def)). It is persisted + mapped even when the item's
                    // LIFETIME owner (its inventory) is unnamed and therefore unpersisted.
                    auto bBuildViaContextOwner = false;
                    auto OwnerSavedId = Entry.Get_ContextOwnerSavedId();
                    if (OwnerSavedId != ck_snapshot_subsystem::k_NoEntity && _PersistedIds.Contains(OwnerSavedId))
                    { bBuildViaContextOwner = true; }
                    else
                    { OwnerSavedId = Entry.Get_LifetimeOwnerSavedId(); }

                    if (OwnerSavedId == ck_snapshot_subsystem::k_NoEntity)
                    {
                        ck::snapshot::Error(TEXT("v3 load: DefinitionBuilt entity [{}] carries no owner recipe — orphaned"), SavedId);
                        DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::NoOwnerRecipe);
                        break;
                    }
                    if (NOT _PersistedIds.Contains(OwnerSavedId))
                    {
                        // Data loss (the item is dropped), NOT boot-infra — flag it loudly rather than skip silently.
                        // The owner must persist: a named inventory, or a persisted context owner.
                        ck::snapshot::Warning(
                            TEXT("v3 load: DefinitionBuilt entity [{}] owner saved-id [{}] was not persisted — item dropped."),
                            SavedId, OwnerSavedId);
                        DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::OwnerNotPersisted);
                        break;
                    }
                    const auto* MappedOwner = _SavedIdMap.Find(OwnerSavedId);
                    if (MappedOwner == nullptr || ck::Is_NOT_Valid(*MappedOwner))
                    {
                        AnyUnresolved = true; // owner not mapped yet — retry next tick (owners precede dependents)
                        break;
                    }

                    auto ConstructionInfos = TArray<FCk_EntityReplicationDriver_ConstructionInfo>{};
                    for (const auto& Step : Entry.Get_BuildRecipe())
                    {
                        auto* ScriptClass = FSoftClassPath{Step.Get_ScriptClassPath()}.TryLoadClass<UCk_Entity_ConstructionScript_PDA>();
                        if (ScriptClass == nullptr)
                        {
                            ck::snapshot::Error(TEXT("v3 load: DefinitionBuilt entity [{}] construction script [{}] unloadable — step dropped"),
                                SavedId, Step.Get_ScriptClassPath());
                            continue;
                        }
                        auto Info = FCk_EntityReplicationDriver_ConstructionInfo{ScriptClass};
                        if (const auto& ArchetypePath = Step.Get_ArchetypePath(); NOT ArchetypePath.IsEmpty())
                        {
                            if (auto* Archetype = Cast<UCk_Entity_ConstructionScript_PDA>(FSoftObjectPath{ArchetypePath}.TryLoad()))
                            { Info.Set_ConstructionScriptArchetype(Archetype); }
                        }
                        ConstructionInfos.Emplace(MoveTemp(Info));
                    }
                    if (ConstructionInfos.IsEmpty())
                    {
                        ck::snapshot::Error(TEXT("v3 load: DefinitionBuilt entity [{}] has no loadable construction steps — orphaned"), SavedId);
                        DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::NoLoadableSteps);
                        break;
                    }

                    // Via the lifetime owner ⇒ mirror production by building under ITS context owner.
                    auto BuildOwner = bBuildViaContextOwner
                        ? *MappedOwner
                        : UCk_Utils_ContextOwner_UE::Get_ContextOwner(*MappedOwner);
                    if (ck::Is_NOT_Valid(BuildOwner))
                    { BuildOwner = *MappedOwner; }

                    if (_RuntimeEntityScriptsAwaitingConstruction.Contains(BuildOwner))
                    {
                        AnyUnresolved = true;
                        break;
                    }

                    // Mapping is an identity rendezvous, not a construction-readiness guarantee. EngineOwned level
                    // roots can publish their SaveKey while their EntityScript pipeline is still composing the
                    // ReplicationDriver required by a DefinitionBuilt child. Wait on that exact prerequisite and let
                    // the existing bounded rebuild/escalation loop diagnose a topology that never becomes ready.
                    const auto BuildOwnerIsReplicated = UCk_Utils_Net_UE::Has(BuildOwner)
                        && UCk_Utils_Net_UE::Get_EntityReplication(BuildOwner) == ECk_Replication::Replicates;
                    if (BuildOwnerIsReplicated && NOT UCk_Utils_EntityReplicationDriver_UE::Has(BuildOwner))
                    {
                        AnyUnresolved = true;
                        break;
                    }

                    const auto BuiltItem = UCk_Utils_EntityReplicationDriver_UE::Request_BuildAndReplicate_Multiple(BuildOwner, ConstructionInfos);

                    // Claim the once-guard + map ONLY on a valid build. An invalid handle (host gate / rep-driver
                    // ensure) must not leave the entry permanently orphaned with no diagnostic.
                    if (ck::IsValid(BuiltItem))
                    {
                        _SpawnedRuntimeIds.Add(SavedId);
                        Resolved = BuiltItem;
                    }
                    else
                    {
                        ck::snapshot::Error(
                            TEXT("v3 load: DefinitionBuilt entity [{}] build under owner [{}] returned an invalid handle — orphaned"),
                            SavedId, BuildOwner);
                        DoRecord_Skip(Entry, ECk_Snapshot_SkipReason::BuildFailed);
                    }
                }
                break;
            }
        }

        if (ck::IsValid(Resolved))
        {
            if (Entry.Get_Provenance() == ECk_Snapshot_V3_Provenance::RuntimeSpawned &&
                Entry.Get_SaveKey().IsValid())
            {
                // A current live entity may have resolved this row through a
                // historical alias. Preserve its canonical key and aliases so
                // the next capture writes the current identity; only rebuilt
                // entities with no authored key adopt the saved key directly.
                if (NOT Resolved.Has<FFragment_SaveKey>())
                { Resolved.Add<FFragment_SaveKey>(Entry.Get_SaveKey()); }
                TryPublish_SaveKey(Entry.Get_SaveKey(), Resolved);
            }

            _SavedIdMap.Add(SavedId, Resolved);
            _MappedLiveEntities.Add(Resolved);
        }
        else if (NOT _SkippedIds.Contains(SavedId))
        { AnyUnresolved = true; } // still pending (bridge linking, owner not yet mapped) — retry next tick
    }

    // Drain kernel work (construction cascades, actor bridges) so pending spawns resolve next tick. No payloads are
    // enqueued yet, so this pump never applies hydration — it only settles construction (avoids the Setup-stomp).
    EcsWorld->Request_PumpToQuiescence(ck::ECk_SchedulerTickScope::LoadKernel);

    return NOT AnyUnresolved;
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    DoOnRuntimeEntityScriptConstructed(
        FCk_Handle_EntityScript InEntityScript)
{
    _RuntimeEntityScriptsAwaitingConstruction.Remove(InEntityScript.ConvertToHandle());
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Snapshot_Subsystem_UE::
    DoOnRuntimeEntityScriptSpawnRequestCompleted(
        FCk_Handle InEntity,
        ECk_Request_OperationResult InResult)
{
    // Failed_NotEnqueued fires synchronously with the lifetime owner and never
    // enters this set. Queued failures report the under-construction entity.
    const auto FailedAfterEnqueue = InResult == ECk_Request_OperationResult::Failed
        || InResult == ECk_Request_OperationResult::Failed_Cancelled;
    if (FailedAfterEnqueue)
    { _RuntimeEntityScriptsAwaitingConstruction.Remove(InEntity); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRecord_Skip(
        const FCk_Snapshot_V3_EntityEntry& InEntry,
        ECk_Snapshot_SkipReason InReason)
    -> void
{
    const auto SavedId = InEntry.Get_SavedId();
    _SkippedIds.Add(SavedId);

    auto Record = FCk_Snapshot_SkipRecord{};
    Record.Set_SavedId(SavedId);
    Record.Set_Provenance(InEntry.Get_Provenance());
    Record.Set_Identity(ck::snapshot::Get_IdentityText(InEntry));
    Record.Set_OwnerSavedId(InEntry.Get_LifetimeOwnerSavedId());
    Record.Set_Reason(InReason);
    _SkipRecords.Add(MoveTemp(Record));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRestore_SavedOwnership()
    -> void
{
    // Endpoints absent from _SavedIdMap retain their valid rebuild-time relationship.
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        if (Entry.Get_Provenance() != ECk_Snapshot_V3_Provenance::RuntimeSpawned &&
            Entry.Get_Provenance() != ECk_Snapshot_V3_Provenance::DefinitionBuilt)
        { continue; }

        const auto* Mapped = _SavedIdMap.Find(Entry.Get_SavedId());
        if (Mapped == nullptr || ck::Is_NOT_Valid(*Mapped))
        { continue; }
        auto Entity = *Mapped;

        if (const auto OwnerSavedId = Entry.Get_LifetimeOwnerSavedId();
            OwnerSavedId != ck_snapshot_subsystem::k_NoEntity)
        {
            const auto* SavedOwner = _SavedIdMap.Find(OwnerSavedId);
            if (SavedOwner != nullptr && ck::IsValid(*SavedOwner) &&
                Entity.Has<ck::FFragment_LifetimeOwner>())
            {
                const auto CurrentOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Entity);
                if (CurrentOwner != *SavedOwner)
                { UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(Entity, *SavedOwner); }
            }
        }

        if (const auto ContextSavedId = Entry.Get_ContextOwnerSavedId();
            ContextSavedId != ck_snapshot_subsystem::k_NoEntity)
        {
            const auto* SavedContext = _SavedIdMap.Find(ContextSavedId);
            if (SavedContext != nullptr && ck::IsValid(*SavedContext))
            {
                const auto CurrentContext = UCk_Utils_ContextOwner_UE::Has(Entity)
                    ? UCk_Utils_ContextOwner_UE::Get_ContextOwner(Entity)
                    : FCk_Handle{};

                if (CurrentContext != *SavedContext)
                { UCk_Utils_ContextOwner_UE::Request_Override(Entity, *SavedContext, {}); }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoApply_SavedTransforms()
    -> void
{
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        // Bridged actors already respawn AT their saved transform (_ActorSpawnTransform seeds the actor spawn) and
        // re-applying would fight it. This is the ONLY guard against double-applying them.
        if (NOT Entry.Get_ActorClassPath().IsEmpty())
        { continue; }

        // Identity is the "no Transform fragment" default (capture only writes the column when the entity Has one).
        // Identity -> Identity is a no-op, so skipping it also avoids stomping an actor whose entity carries no Transform.
        const auto& Saved = Entry.Get_SavedWorldTransform();
        if (Saved.Equals(FTransform::Identity))
        { continue; }

        const auto* Mapped = _SavedIdMap.Find(Entry.Get_SavedId());
        if (Mapped == nullptr || ck::Is_NOT_Valid(*Mapped))
        { continue; }
        auto Entity = *Mapped;

        // Actor-backed entity (EngineOwned player pawn, EngineOwned SaveKey level actor): drive the ACTOR only. NEVER
        // the entity-side Transform — FProcessor_Transform_SyncFromActor stomps it back from the actor every tick.
        if (auto* Actor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(Entity);
            ck::IsValid(Actor))
        {
            constexpr auto bSweep = false;
            Actor->SetActorTransform(Saved, bSweep, nullptr, ETeleportType::TeleportPhysics);
            continue;
        }

        // Pure-ECS mover (no owning actor): Request_SetTransform drains in the load-kernel pumps AFTER any
        // Construct-seeded transform requests (FIFO), so the saved value wins.
        if (UCk_Utils_Transform_UE::Has(Entity))
        {
            UCk_Utils_Transform_TypeUnsafe_UE::Request_SetTransform(
                Entity, FCk_Request_Transform_SetTransform{Saved}, {});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoHydrate_Enqueue()
    -> void
{
    if (_HydrationEnqueued)
    { return; }
    _HydrationEnqueued = true;

    DoRestore_SavedOwnership();
    DoApply_SavedTransforms();

    // Game-side rebind processors key off FTag_Hydration_WasHydratedThisLoad to re-resolve handles their persisted
    // fragments carry; stamp it before the gate opens so the full post-gate pump sees it. It is never removed.
    for (const auto& Pair : _SavedIdMap)
    {
        if (auto Restored = Pair.Value;
            ck::IsValid(Restored))
        { Restored.AddOrGet<ck::FTag_Hydration_WasHydratedThisLoad>(); }
    }

    // Quarantine the mapped set HERE, beside that stamp, not at row mapping: a RuntimeSpawned row is mapped while
    // still constructing and its finisher is a gated GAME processor, so excluding mapped entities during Rebuilding
    // starves the very processor the rebuild is waiting on. From this point the rebuild is done and the load owns
    // these entities' Durable state until the whole set is released together.
    DoStamp_HydrationQuarantine();

    // An orphan is a saved entity that never mapped AND was not deliberately skipped — skips are intentional (the
    // fresh world's boot owns them). Computed before the payload walk so each dropped payload is attributed to the
    // bucket its OWNER landed in rather than to an anonymous continue.
    auto OrphanIds = TSet<uint32>{};
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        const auto SavedId = Entry.Get_SavedId();
        if (NOT _SavedIdMap.Contains(SavedId) && NOT _SkippedIds.Contains(SavedId))
        { OrphanIds.Add(SavedId); }
    }

    auto EnqueuedCount = 0;
    auto PayloadsDropped = 0;
    auto PayloadsOnSkipped = 0;
    auto PayloadsOnOrphaned = 0;
    auto PayloadsOnUnresolvedOwner = 0;
    for (const auto& Payload : _V3Tables.Get_Payloads())
    {
        const auto OwnerSavedId = Payload.Get_OwnerSavedId();
        auto* Owner = _SavedIdMap.Find(OwnerSavedId);
        if (Owner == nullptr || ck::Is_NOT_Valid(*Owner))
        {
            // Owner unmapped — its payloads drop. Which bucket says WHY: a skipped owner is deliberate, an orphaned
            // one is lost state, and neither means the owner id is absent from the entity table or its mapped handle
            // died between rebuild and here.
            if (_SkippedIds.Contains(OwnerSavedId))
            { ++PayloadsOnSkipped; }
            else if (OrphanIds.Contains(OwnerSavedId))
            { ++PayloadsOnOrphaned; }
            else                                       { ++PayloadsOnUnresolvedOwner; }
            continue;
        }

        // A failed deserialize is lost state, not a no-op — counted in the load report even where the ensure is out.
        auto Data = DoDeserialize_V3Blob(Payload.Get_PayloadBytes());
        const auto HasHydrationData = Data.IsValid();
        CK_ENSURE_IF_NOT(HasHydrationData,
            TEXT("v3 load: hydration payload for type [{}] (owner saved-id [{}]) failed to deserialize — dropped "
                 "(empty bytes, or the type is absent since the save)"),
            Payload.Get_TypePath(), Payload.Get_OwnerSavedId())
        {
            ++PayloadsDropped;
            continue;
        }

        auto Entity = *Owner;
        Entity.AddOrGet<ck::FFragment_PendingHydration>().Enqueue(GetWorld(), MoveTemp(Data));
        if (NOT Entity.Has<ck::FTag_Hydration_PendingApply>())
        { Entity.Add<ck::FTag_Hydration_PendingApply>(); }
        ++EnqueuedCount;
    }

    // Each orphan gets one Warning + one report record.
    auto Orphans = TArray<FCk_Snapshot_OrphanRecord>{};
    Orphans.Reserve(OrphanIds.Num());
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        const auto SavedId = Entry.Get_SavedId();
        if (NOT OrphanIds.Contains(SavedId))
        { continue; }

        const auto OwnerSavedId  = Entry.Get_LifetimeOwnerSavedId();
        const auto bOwnerOrphaned = OwnerSavedId != ck_snapshot_subsystem::k_NoEntity && OrphanIds.Contains(OwnerSavedId);

        const auto Identity = ck::snapshot::Get_IdentityText(Entry);
        auto Reason = FString{};
        switch (Entry.Get_Provenance())
        {
            case ECk_Snapshot_V3_Provenance::EngineOwned:
            {
                Reason = Entry.Get_SaveKey().IsValid() ? TEXT("savekey-miss") : TEXT("player-miss");
                break;
            }
            case ECk_Snapshot_V3_Provenance::ConstructSpawned:
            {
                if (bOwnerOrphaned)                          { Reason = TEXT("owner-orphaned"); }          // cascade
                else if (_SavedIdMap.Contains(OwnerSavedId)) { Reason = TEXT("owner-mapped-label-miss"); } // content/label drift
                else                                         { Reason = TEXT("unresolved-other"); }
                break;
            }
            case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
            {
                if (NOT Entry.Get_ActorClassPath().IsEmpty()) { Reason = TEXT("bridge-never-linked"); } // actor spawned, bridge never linked
                else if (bOwnerOrphaned)
                { Reason = TEXT("owner-orphaned"); }
                else                                          { Reason = TEXT("unresolved-other"); }
                break;
            }
            case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
            {
                if (bOwnerOrphaned)
                { Reason = TEXT("owner-orphaned"); }
                else                { Reason = TEXT("unresolved-other"); }
                break;
            }
        }

        ck::snapshot::Warning(TEXT("v3 load ORPHAN: saved-id [{}] provenance [{}] identity [{}] owner [{}] reason [{}]"),
            SavedId, ck::snapshot::Get_ProvenanceText(Entry.Get_Provenance()), Identity, OwnerSavedId, Reason);

        auto Record = FCk_Snapshot_OrphanRecord{};
        Record.Set_SavedId(SavedId);
        Record.Set_Provenance(Entry.Get_Provenance());
        Record.Set_Identity(Identity);
        Record.Set_OwnerSavedId(OwnerSavedId);
        Record.Set_Reason(Reason);
        Orphans.Add(MoveTemp(Record));
    }

    _V3LoadReport.Set_EntitiesTotal(_V3Tables.Get_Entities().Num());
    _V3LoadReport.Set_EntitiesRestored(_SavedIdMap.Num());
    _V3LoadReport.Set_EntitiesSkipped(_SkippedIds.Num());
    _V3LoadReport.Set_EntitiesOrphaned(OrphanIds.Num());
    _V3LoadReport.Set_Orphans(MoveTemp(Orphans));
    _V3LoadReport.Set_Skips(_SkipRecords);
    _V3LoadReport.Set_PayloadsTotal(_V3Tables.Get_Payloads().Num());
    _V3LoadReport.Set_PayloadsEnqueued(EnqueuedCount);
    _V3LoadReport.Set_PayloadsOnSkippedEntities(PayloadsOnSkipped);
    _V3LoadReport.Set_PayloadsOnOrphanedEntities(PayloadsOnOrphaned);
    _V3LoadReport.Set_PayloadsOnUnresolvedOwner(PayloadsOnUnresolvedOwner);
    _V3LoadReport.Set_PayloadsDropped(PayloadsDropped);

    ck::snapshot::Display(
        TEXT("DIAG: v3 hydrate — entities [{}] = mapped [{}] + skipped [{}] + orphaned [{}] | ")
        TEXT("payloads [{}] = enqueued [{}] + on-skipped [{}] + on-orphaned [{}] + unresolved-owner [{}] + dropped [{}]"),
        _V3LoadReport.Get_EntitiesTotal(), _V3LoadReport.Get_EntitiesRestored(), _V3LoadReport.Get_EntitiesSkipped(),
        _V3LoadReport.Get_EntitiesOrphaned(), _V3LoadReport.Get_PayloadsTotal(), EnqueuedCount, PayloadsOnSkipped,
        PayloadsOnOrphaned, PayloadsOnUnresolvedOwner, PayloadsDropped);

    // No accounting ensure here any more: the payload closure now asks what each row RESULTED IN, and at enqueue
    // time nothing has been applied yet. It is checked once the outcomes are folded, in DoFinish_Load.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    Get_IsHydrationPending(
        const FCk_Handle& InHandle) const
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    if (NOT _LoadInProgress || _QuarantineLifted)
    { return false; }

    return _MappedLiveEntities.Contains(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_HydrationComplete() const
    -> bool
{
    // Drain AND release. The lift gates on DoIs_PayloadDrainComplete alone; if this predicate also gated the lift
    // it would be waiting on itself, and the settle could only ever exit through the frame cap.
    return DoIs_PayloadDrainComplete() && NOT _QuarantineStamped;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_PayloadDrainComplete() const
    -> bool
{
    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return true; }

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return true; }

    auto& CkRegistry = EcsWorld->Get_Registry();
    auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
    if (RawRegistry == nullptr)
    { return true; }

    // in_place storage → iterate (view::empty() is SFINAE-disabled); any yielded entity means a payload is pending.
    for (const auto Entity : RawRegistry->view<ck::FTag_Hydration_PendingApply>())
    {
        (void)Entity;
        return false;
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    Request_AddLoadCompletePromise(
        const FCk_Delegate_Snapshot_OnLoadComplete& InDelegate)
    -> void
{
    if (NOT InDelegate.IsBound())
    { return; }

    _PendingLoadCompletePromises.Emplace(InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_HydrateFrameCap() const
    -> int32
{
#if WITH_AUTOMATION_TESTS
    if (_TestOnly_HydrateFrameCapOverride > 0)
    { return _TestOnly_HydrateFrameCapOverride; }
#endif
    return kLoad_HydrateFrameCap;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_ConvergenceFrameCap() const
    -> int32
{
#if WITH_AUTOMATION_TESTS
    if (_TestOnly_ConvergenceFrameCapOverride > 0)
    { return _TestOnly_ConvergenceFrameCapOverride; }
#endif
    return kLoad_ConvergenceFrameCap;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoStamp_HydrationQuarantine()
    -> void
{
    auto StampedCount = 0;
    auto Registry = FCk_Registry{};

    for (auto Restored : _MappedLiveEntities)
    {
        if (ck::Is_NOT_Valid(Restored))
        { continue; }

        Restored.AddOrGet<ck::FTag_Hydration_Quarantine>();
        Registry = Restored.Get_RegistryView();
        ++StampedCount;
    }

    if (StampedCount == 0)
    { return; }

    Registry.SetContext<ck::FCtx_HydrationQuarantine>()._Count = StampedCount;
    _QuarantineStamped = true;

    ck::snapshot::Display(TEXT("DIAG: v3 hydrate — quarantined [{}] restored entities until every payload applies"),
        StampedCount);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoLift_HydrationQuarantine(
        EQuarantineLift InReason,
        FCk_Snapshot_LoadReport& InOutReport)
    -> void
{
    if (NOT _QuarantineStamped)
    { return; }

    const auto Forced = InReason != EQuarantineLift::Settled;
    const auto* ReasonText = InReason == EQuarantineLift::ForcedAtFrameCap
        ? TEXT("hydrate-frame-cap")
        : TEXT("load-finish");

    auto ReleasedCount = 0;
    auto ForcedRecords = TArray<FCk_Snapshot_QuarantineForcedRecord>{};
    auto Registry = FCk_Registry{};

    // Collected in pass 1, broadcast in pass 2. Two passes for the same reason the dynamic-fragment hydrator
    // commits every value before its first notification: a listener that runs while the sweep is half-done sees
    // some of the set released and the rest still held, which is precisely the torn cross-entity read the global
    // lift exists to prevent. It also lets the forced-release records reach the report BEFORE the entities they
    // name get their edge, so a consumer that reacts to OnHydrated by reading the report is not told a lie.
    auto ReleasedHandles = TArray<FCk_Handle>{};
    ReleasedHandles.Reserve(_MappedLiveEntities.Num());

    for (auto Restored : _MappedLiveEntities)
    {
        // The set is append-only across the load and is never pruned when an entity is destroyed mid-load, so it
        // retains stale handles. Removing a fragment through one ensures — skip them rather than storm.
        if (ck::Is_NOT_Valid(Restored))
        { continue; }

        Registry = Restored.Get_RegistryView();

        if (NOT Restored.Try_Remove<ck::FTag_Hydration_Quarantine>())
        { continue; }

        ++ReleasedCount;

        // The once-guard for OnHydrated is this branch and nothing else: only an entity whose tag THIS pass
        // actually removed gets an edge. A second escape running after a first lift finds nothing to remove and
        // so broadcasts nothing, which is what keeps "exactly once per entity per load" true across all three
        // release sites without a separate ledger to keep in sync.
        ReleasedHandles.Emplace(Restored);

        if (NOT Forced || NOT Restored.Has<ck::FTag_Hydration_PendingApply>())
        { continue; }

        // Forced out with payloads still queued: that IS the loss, and it is named per entity rather than summarised,
        // because "some payloads did not apply" tells nobody which part of their world came back wrong.
        const auto Outstanding = Restored.Has<ck::FFragment_PendingHydration>()
            ? Restored.Get<ck::FFragment_PendingHydration>().Get_Entries().Num()
            : 0;

        auto Record = FCk_Snapshot_QuarantineForcedRecord{};
        Record.Set_Identity(ck::Format_UE(TEXT("{}"), Restored));
        Record.Set_PayloadsOutstanding(Outstanding);
        Record.Set_Reason(FString{ReasonText});
        ForcedRecords.Emplace(MoveTemp(Record));

        ck::snapshot::Error(TEXT("Request_Load: entity [{}] was released from the hydration quarantine by the [{}] "
            "escape with [{}] payload entries still queued — those payloads never applied and its restored state is "
            "incomplete"), Restored, ReasonText, Outstanding);
    }

    if (ReleasedCount > 0)
    { Registry.SetContext<ck::FCtx_HydrationQuarantine>()._Count = 0; }

    _QuarantineStamped = false;

    // Before the broadcast, so a listener that binds Promise_OnHydrated on some OTHER entity from inside its
    // callback is told the truth — nothing is pending any more — instead of waiting on an edge that has passed.
    _QuarantineLifted = true;

    if (Forced)
    {
        const auto ForcedCount = ForcedRecords.Num();
        InOutReport.Set_QuarantineForced(MoveTemp(ForcedRecords));

        ck::snapshot::Error(TEXT("Request_Load: hydration quarantine FORCED off ([{}]) — released [{}] entities, [{}] "
            "of them with payloads still pending"), ReasonText, ReleasedCount, ForcedCount);
    }
    else
    {
        ck::snapshot::Display(TEXT("DIAG: v3 hydrate — quarantine lifted for [{}] entities (every payload applied)"),
            ReleasedCount);
    }

    // Pass 2. Every subscriber now observes a set that is entirely released, and — on a forced lift — a report
    // that already names what was lost. Re-validated per entity because a listener may destroy entities that
    // appear later in this list.
    for (auto Released : ReleasedHandles)
    {
        if (ck::Is_NOT_Valid(Released))
        { continue; }

        ck::UUtils_Signal_Hydration_OnHydrated::Broadcast(Released, ck::MakePayload(Released));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoFold_HydrationOutcomes(
        FCk_Snapshot_LoadReport& InOutReport) const
    -> void
{
    auto* EcsWorld = DoGet_LoadWorldEcs();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return; }

    auto& CkRegistry = EcsWorld->Get_Registry();

    if (const auto* Outcomes = CkRegistry.TryGetContext<ck::FCtx_HydrationOutcomes>())
    {
        InOutReport.Set_PayloadsApplied(Outcomes->_Applied);
        InOutReport.Set_PayloadsRejected(Outcomes->_Rejected);
        InOutReport.Set_PayloadsDroppedNoHandler(Outcomes->_DroppedNoHandler);
        InOutReport.Set_PayloadsDroppedTimeout(Outcomes->_DroppedTimeout);
        InOutReport.Set_PayloadsDestroyedWithEntries(Outcomes->_DestroyedWithEntries);

        auto Losses = TArray<FCk_Snapshot_PayloadLossRecord>{};
        Losses.Reserve(Outcomes->_Losses.Num());
        for (const auto& Loss : Outcomes->_Losses)
        {
            auto Record = FCk_Snapshot_PayloadLossRecord{};
            Record.Set_PayloadType(Loss._PayloadType);
            Record.Set_OwnerIdentity(Loss._OwnerIdentity);
            Record.Set_Reason(Loss._Reason);
            Losses.Emplace(MoveTemp(Record));
        }
        InOutReport.Set_PayloadLosses(MoveTemp(Losses));
    }

    auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
    if (RawRegistry == nullptr)
    { return; }

    // Whatever is still queued right now. Entities already entering destruction are skipped: their entries were
    // written off (and removed) at destroy-initiate, and counting them again here would break the closure by
    // putting one payload row in two buckets.
    auto UnappliedAtFinish = 0;
    for (const auto Entity : RawRegistry->view<ck::FTag_Hydration_PendingApply>())
    {
        auto Handle = ck::MakeHandle(FCk_Entity{Entity}, CkRegistry);
        if (ck::Is_NOT_Valid(Handle) || NOT Handle.Has<ck::FFragment_PendingHydration>())
        { continue; }

        if (Handle.Has_Any<ck::FTag_DestroyEntity_Initiate, ck::FTag_DestroyEntity_EndPlay,
                           ck::FTag_DestroyEntity_Teardown, ck::FTag_DestroyEntity_Await,
                           ck::FTag_DestroyEntity_Finalize>())
        { continue; }

        UnappliedAtFinish += Handle.Get<ck::FFragment_PendingHydration>().Get_Entries().Num();
    }

    InOutReport.Set_PayloadsUnappliedAtFinish(UnappliedAtFinish);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoCompute_LoadResult(
        FCk_Snapshot_LoadReport& InOutReport) const
    -> void
{
    // A load that did not complete keeps saying so. This only ever answers the narrower question of whether a
    // COMPLETED load completed intact.
    if (InOutReport.Get_Result() != ECk_SnapshotResult::Success)
    { return; }

    const auto LostPayloads =
        InOutReport.Get_PayloadsRejected() + InOutReport.Get_PayloadsDroppedNoHandler() +
        InOutReport.Get_PayloadsDroppedTimeout() + InOutReport.Get_PayloadsDestroyedWithEntries() +
        InOutReport.Get_PayloadsUnappliedAtFinish() + InOutReport.Get_PayloadsDropped();

    const auto ForcedReleases = InOutReport.Get_QuarantineForced().Num();

    // A convergence fact the load gave up waiting on is a loss of the same kind: the world was handed back in a
    // state the load promised it would not be handed back in. It is NAMED, it is bounded, and it must not read
    // as Success — a caller that resumes gameplay on Success would be resuming onto exactly the unsettled world
    // the hold exists to hide.
    const auto ConvergenceUnmet = InOutReport.Get_ConvergenceUnmet().Num();

    // _EntitiesOrphaned and _UnresolvedAfterEscalation are deliberately NOT in this set. Orphans are a routine
    // outcome of the current loader — they have their own per-row Warning and records — so including them would
    // make virtually every load report a loss and drain the distinction of meaning.
    if (LostPayloads == 0 && ForcedReleases == 0 && ConvergenceUnmet == 0)
    { return; }

    InOutReport.Set_Result(ECk_SnapshotResult::Succeeded_WithLoss);

    ck::snapshot::Error(TEXT("Request_Load: the load COMPLETED WITH LOSS — [{}] payload entries did not apply "
        "(rejected [{}], no handler [{}], timed out [{}], destroyed with their entity [{}], still queued at finish "
        "[{}], failed to deserialize [{}]), [{}] entities were forced out of the hydration quarantine, and [{}] "
        "convergence facts were never met. The world is playable; the state named below is not in it"),
        LostPayloads, InOutReport.Get_PayloadsRejected(), InOutReport.Get_PayloadsDroppedNoHandler(),
        InOutReport.Get_PayloadsDroppedTimeout(), InOutReport.Get_PayloadsDestroyedWithEntries(),
        InOutReport.Get_PayloadsUnappliedAtFinish(), InOutReport.Get_PayloadsDropped(), ForcedReleases,
        ConvergenceUnmet);

    for (const auto& Loss : InOutReport.Get_PayloadLosses())
    {
        ck::snapshot::Error(TEXT("Request_Load: LOST payload [{}] on entity [{}] — reason [{}]"),
            Loss.Get_PayloadType(), Loss.Get_OwnerIdentity(), Loss.Get_Reason());
    }

    for (const auto& Unmet : InOutReport.Get_ConvergenceUnmet())
    {
        ck::snapshot::Error(TEXT("Request_Load: UNMET convergence [{}] — still pending after [{}] frames; the world "
            "resumed without it"), Unmet.Get_Name(), Unmet.Get_FramesWaited());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoApply_TimeFreeze(
        UWorld& InWorld)
    -> void
{
    auto* Settings = InWorld.GetWorldSettings();
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    // Already frozen in THIS world. Re-applying would capture the floor AS the prior value, so the restore would
    // hand the world back a permanent freeze. A DIFFERENT world re-arms rather than no-ops: the freeze is per
    // world, and the post-travel world arrives at 1.0 no matter what the pre-travel world was set to.
    if (_TimeFreezeWorld.Get() == &InWorld)
    { return; }

    // The one project-level off-switch, and it defaults true. If a world ever turns it off, the freeze is inert
    // and has to say so — every promise the load makes about time not advancing is void on that world.
    CK_ENSURE_IF_NOT(Settings->bAllowTimeDilation,
        TEXT("CkSnapshot's load-time freeze is INERT on world [{}]: bAllowTimeDilation is false there, so global "
             "time dilation is ignored and GAME TIME WILL ADVANCE for the whole load — timers, cadences and "
             "world-time deadlines will fire against a world the player cannot see"),
        InWorld.GetName())
    { }

    _TimeFreezeWorld   = &InWorld;
    _PriorTimeDilation = Settings->TimeDilation;

    Settings->SetTimeDilation(Settings->MinGlobalTimeDilation);

    // Copied out before it is formatted: bAllowTimeDilation is a BITFIELD (WorldSettings.h:468), and a bitfield
    // lvalue cannot bind to the non-const reference the formatter's forwarding parameter deduces.
    const auto AllowsTimeDilation = static_cast<bool>(Settings->bAllowTimeDilation);

    // Assert the OUTCOME, not the request. SetTimeDilation clamps into [Min,Max]GlobalTimeDilation, so asking
    // for the floor always succeeds and returns whatever the floor is — and the floor is authorable per level.
    CK_ENSURE_IF_NOT(Settings->GetEffectiveTimeDilation() <= ck_snapshot_subsystem::k_MaxFrozenTimeDilation,
        TEXT("CkSnapshot's load-time freeze did NOT take on world [{}]: effective time dilation is [{}] after "
             "writing the floor [{}]. Game time will keep advancing through the load. Check that world's "
             "MinGlobalTimeDilation (World Settings, saved into the map) and bAllowTimeDilation [{}]"),
        InWorld.GetName(), Settings->GetEffectiveTimeDilation(), Settings->MinGlobalTimeDilation,
        AllowsTimeDilation)
    { }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRestore_TimeFreeze(
        UWorld& InWorld)
    -> void
{
    if (_TimeFreezeWorld.Get() != &InWorld)
    {
        // Restoring a world that was never frozen is the ordinary case on every exit route that runs without a
        // freeze in flight. Being frozen in a DIFFERENT live world while someone restores this one is not: it
        // means an exit route is about to leave that world dilated with nobody left to release it.
        if (const auto* FrozenWorld = _TimeFreezeWorld.Get();
            FrozenWorld != nullptr)
        {
            ck::snapshot::Warning(TEXT("Restoring the load-time freeze on world [{}], but it is held on world [{}] — "
                "that world is about to be left with game time frozen"), InWorld.GetName(), FrozenWorld->GetName());
        }
        return;
    }

    _TimeFreezeWorld.Reset();

    auto* Settings = InWorld.GetWorldSettings();
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    // Anything other than the floor means something else moved the dial while the load held it. Say so rather
    // than clobber silently — the value being overwritten is that writer's, not ours.
    if (NOT FMath::IsNearlyEqual(Settings->TimeDilation, Settings->MinGlobalTimeDilation))
    {
        ck::snapshot::Warning(TEXT("Time dilation on world [{}] changed during the load hold (found [{}], expected the "
            "floor [{}]) — restoring [{}] anyway"), InWorld.GetName(), Settings->TimeDilation,
            Settings->MinGlobalTimeDilation, _PriorTimeDilation);
    }

    Settings->SetTimeDilation(_PriorTimeDilation);
    _PriorTimeDilation = 1.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoSet_LoadHold(
        ECk_EcsWorld_LoadHold InHold)
    -> void
{
    // A world that has gone away is not an error here: a load's own travel is exactly that, and the phase whose
    // transition asked for this hold is about to raise it again on the world that replaces it.
    if (auto* EcsWorld = DoGet_LoadWorldEcs();
        ck::IsValid(EcsWorld))
    { EcsWorld->Set_LoadHold(InHold); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoDrive_Convergence()
    -> void
{
    auto* EcsWorld = DoGet_LoadWorldEcs();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return; }

    auto& Registry = EcsWorld->Get_Registry();

    // The registered advances first: physics is frozen with everything else, so without a grant its bodies never
    // move and the predicates waiting on them wait forever. Each advance stamps its own baseline on this frame if
    // it needs one, which is why they run BEFORE _FramesConverging is incremented.
    ck::FCk_LoadConvergenceRegistry::Run_Advances(Registry);

    const auto PumpCount = EcsWorld->Request_PumpToQuiescence(ck::ECk_SchedulerTickScope::Full);

    if (auto* Convergence = Registry.TryGetContext<ck::FCtx_LoadConvergence>();
        Convergence != nullptr)
    {
        Convergence->_PumpCountLastFrame = PumpCount;
        Convergence->_PumpSkippedGroupsLastFrame = EcsWorld->Get_LastPumpSkippedGroupCount();
        ++Convergence->_FramesConverging;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_ConvergenceGrantedSteps() const
    -> int32
{
    const auto* EcsWorld = DoGet_LoadWorldEcs();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return 0; }

    const auto* Convergence = EcsWorld->Get_Registry().TryGetContext<ck::FCtx_LoadConvergence>();
    return Convergence != nullptr ? Convergence->_PhysicsGrantedStepsSinceHold : 0;
}

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_ConvergenceSeriesText(
        const TArray<int32>& InSeries) const
    -> FString
{
    auto Parts = TArray<FString>{};
    Parts.Reserve(InSeries.Num());
    for (const auto Value : InSeries)
    { Parts.Emplace(FString::FromInt(Value)); }

    return FString::Join(Parts, TEXT(","));
}

auto
    UCk_Snapshot_Subsystem_UE::
    DoReport_ConvergenceProgress(
        const TArray<FName>& InPending)
    -> void
{
    const auto GrantedSteps = DoGet_ConvergenceGrantedSteps();

    // The edge, once per row per load. This is the line that answers "when did physics actually catch up" and
    // "which fact was the slow one" without anybody having to reproduce the load.
    for (const auto& WasPending : _ConvergencePendingLastFrame)
    {
        if (InPending.Contains(WasPending))
        { continue; }

        ck::snapshot::Display(TEXT("DIAG: v3 convergence row [{}] satisfied at frame [{}], granted physics steps "
            "executed [{}]"), WasPending, _LoadFrameCount, GrantedSteps);
    }

    _ConvergencePendingLastFrame.Reset();
    for (const auto& Name : InPending)
    { _ConvergencePendingLastFrame.Add(Name); }

    auto PumpCount = 0;
    auto SkippedCount = 0;
    if (const auto* EcsWorld = DoGet_LoadWorldEcs();
        ck::IsValid(EcsWorld))
    {
        if (const auto* Convergence = EcsWorld->Get_Registry().TryGetContext<ck::FCtx_LoadConvergence>();
            Convergence != nullptr)
        {
            PumpCount = Convergence->_PumpCountLastFrame;
            SkippedCount = Convergence->_PumpSkippedGroupsLastFrame;
        }
    }

    // Bounded by construction: a rolling window, not a transcript. 180 frames of per-frame Display would bury the
    // very log it is meant to make readable.
    _ConvergencePumpSeries.Emplace(PumpCount);
    if (_ConvergencePumpSeries.Num() > kConvergenceSeriesWindow)
    { _ConvergencePumpSeries.RemoveAt(0); }

    _ConvergenceSkippedSeries.Emplace(SkippedCount);
    if (_ConvergenceSkippedSeries.Num() > kConvergenceSeriesWindow)
    { _ConvergenceSkippedSeries.RemoveAt(0); }

    ck::snapshot::Verbose(TEXT("DIAG: v3 convergence frame [{}] — pending [{}], pump [{}], skipped groups [{}], "
        "granted physics steps [{}]"), _LoadFrameCount, InPending.Num(), PumpCount, SkippedCount, GrantedSteps);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoArm_ConvergenceDebugTiming()
    -> void
{
    if (_ConvergenceDebugTimingArmed)
    { return; }

    auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Scheduler.DebugTiming"));
    if (CVar == nullptr)
    { return; }

    _ConvergenceDebugTimingPrior = CVar->GetBool();
    _ConvergenceDebugTimingArmed = true;

    if (_ConvergenceDebugTimingPrior)
    { return; }

    CVar->Set(true, ECVF_SetByCode);

    ck::snapshot::Display(TEXT("DIAG: v3 convergence still pending at frame [{}] — enabling ck.Scheduler.DebugTiming "
        "so the stall report can name the processors keeping the world awake"), _LoadFrameCount);
}

auto
    UCk_Snapshot_Subsystem_UE::
    DoRestore_ConvergenceDebugTiming()
    -> void
{
    if (NOT _ConvergenceDebugTimingArmed)
    { return; }

    _ConvergenceDebugTimingArmed = false;

    if (_ConvergenceDebugTimingPrior)
    { return; }

    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Scheduler.DebugTiming")))
    { CVar->Set(false, ECVF_SetByCode); }
}

auto
    UCk_Snapshot_Subsystem_UE::
    DoReport_ConvergenceStall() const
    -> void
{
    const auto* EcsWorld = DoGet_LoadWorldEcs();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return; }

    // Per tick group, in the order they are pumped, so the report reads the way the frame ran.
    auto OrderedTickGroups = TArray<TEnumAsByte<ETickingGroup>>{};
    EcsWorld->Get_WorldActors().GenerateKeyArray(OrderedTickGroups);
    OrderedTickGroups.Sort([](const TEnumAsByte<ETickingGroup>& InA, const TEnumAsByte<ETickingGroup>& InB) -> bool
    { return InA.GetValue() < InB.GetValue(); });

    for (const auto& TickGroup : OrderedTickGroups)
    {
        const auto& Actor = EcsWorld->Get_WorldActors()[TickGroup];
        if (NOT Actor.IsValid())
        { continue; }

        const auto& SchedulerOpt = Actor->Get_Scheduler();
        if (NOT SchedulerOpt.IsSet())
        { continue; }

        const auto& History = SchedulerOpt.GetValue().Get_DebugFrameHistory();
        if (History.IsEmpty())
        {
            ck::snapshot::Display(TEXT("DIAG: v3 convergence stall — tick group [{}] has no debug frame history "
                "(per-processor timing was not enabled long enough to record one)"), TickGroup);
            continue;
        }

        auto Pumped = TArray<ck::FSchedulerDebug_ProcessorTiming>{};
        for (const auto& Timing : History.Last().ProcessorTimings)
        {
            if (Timing.PumpCountThisFrame > 0)
            { Pumped.Emplace(Timing); }
        }

        if (Pumped.IsEmpty())
        {
            ck::snapshot::Display(TEXT("DIAG: v3 convergence stall — tick group [{}] pumped NO processor on its "
                "last recorded frame"), TickGroup);
            continue;
        }

        Pumped.Sort([](const ck::FSchedulerDebug_ProcessorTiming& InA,
                       const ck::FSchedulerDebug_ProcessorTiming& InB) -> bool
        { return InA.PumpCountThisFrame > InB.PumpCountThisFrame; });

        constexpr auto MaxReported = 16;
        auto Lines = TArray<FString>{};
        for (auto Index = 0; Index < FMath::Min(Pumped.Num(), MaxReported); ++Index)
        {
            const auto& Timing = Pumped[Index];

            auto Counts = TArray<FString>{};
            for (const auto EntityCount : Timing.PumpPassEntityCounts)
            { Counts.Emplace(FString::FromInt(EntityCount)); }

            Lines.Emplace(FString::Printf(TEXT("%s x%d entities[%s]"),
                *Timing.ProcessorName.ToString(), Timing.PumpCountThisFrame, *FString::Join(Counts, TEXT(","))));
        }

        ck::snapshot::Display(TEXT("DIAG: v3 convergence stall — tick group [{}] pumped [{}] processor(s) on its "
            "last recorded frame; top [{}] by pump count: {}"),
            TickGroup, Pumped.Num(), Lines.Num(), FString::Join(Lines, TEXT(" | ")));
    }

    // The other row that hit the cap in practice. A destroy queue that never drains is a set of entities, and
    // naming a few of them is the difference between "something is being destroyed" and a lead.
    auto& CkRegistry = EcsWorld->Get_Registry();
    if (auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle()))
    {
        auto Names = TArray<FString>{};
        auto Total = 0;
        for (const auto Entity : RawRegistry->view<ck::FTag_DestroyEntity_Initiate>())
        {
            ++Total;
            if (Names.Num() >= 8)
            { continue; }

            auto Handle = ck::MakeHandle(FCk_Entity{Entity}, CkRegistry);
            Names.Emplace(ck::Format_UE(TEXT("{}"), Handle));
        }

        if (Total > 0)
        {
            ck::snapshot::Display(TEXT("DIAG: v3 convergence stall — [{}] entit(ies) still carry "
                "FTag_DestroyEntity_Initiate; first [{}]: {}"), Total, Names.Num(), FString::Join(Names, TEXT(" | ")));
        }
    }
}

auto
    UCk_Snapshot_Subsystem_UE::
    DoRecord_ConvergenceUnmet(
        FCk_Snapshot_LoadReport& InOutReport) const
    -> void
{
    const auto* EcsWorld = DoGet_LoadWorldEcs();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return; }

    const auto Pending = ck::FCk_LoadConvergenceRegistry::Get_Pending(EcsWorld->Get_Registry());
    if (Pending.IsEmpty())
    { return; }

    auto Records = InOutReport.Get_ConvergenceUnmet();
    const auto FrameCap = DoGet_ConvergenceFrameCap();

    for (const auto& Name : Pending)
    {
        ck::snapshot::Error(TEXT("Request_Load: convergence fact [{}] never reported converged within [{}] frames. "
            "The world is being handed back anyway — a world resumed early is recoverable, a world never handed "
            "back is not — and this load reports Succeeded_WithLoss because of it"),
            Name, FrameCap);

        Records.Emplace(FCk_Snapshot_ConvergenceLossRecord{}
            .Set_Name(Name)
            .Set_FramesWaited(FrameCap));
    }

    InOutReport.Set_ConvergenceUnmet(Records);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoEnter_ReadyToResume()
    -> void
{
    // Order matters, and this is the whole of it: the world becomes NORMAL before any consumer is told about it.
    // The hold comes off, game time restarts, the screen comes down, and the fact reaches clients — only then
    // does DoFinish_Load close the report and drain the promises, so a callback that spawns, polls or reads a
    // clock sees a world that is running rather than one that is a frame away from it.
    DoSet_LoadHold(ECk_EcsWorld_LoadHold::None);

    if (auto* FrozenWorld = _TimeFreezeWorld.Get();
        FrozenWorld != nullptr)
    { DoRestore_TimeFreeze(*FrozenWorld); }

    _IsReadyToResume = true;
    DoPublish_LoadState(true);

    DoRelease_LoadScreenHold(_LoadScreenHold);

    ck::snapshot::Display(TEXT("DIAG: v3 load READY TO RESUME (epoch [{}], restored [{}], orphaned [{}], convergence "
        "unmet [{}])"), _LoadEpoch, _V3LoadReport.Get_EntitiesRestored(), _V3LoadReport.Get_EntitiesOrphaned(),
        _V3LoadReport.Get_ConvergenceUnmet().Num());

    DoFinish_Load(_V3LoadReport);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoCreate_LoadScreenHold(
        UObject* InWorldContext,
        const FString& InReason,
        TObjectPtr<UCk_LoadingProcess_Task_UE>& InOutHold) const
    -> void
{
    if (ck::IsValid(InOutHold))
    { return; }

    // FCk_Time{} — deliberately NO watchdog. The task's watchdog is wall-clock, and a load does not run at 60 fps:
    // it runs a blocking LoadMap, package loads and PSO warm-up. A timeout sized for a healthy frame rate fires on
    // a healthy slow load and drops the screen over a half-rebuilt world, which is precisely the thing the screen
    // is up for. The loader's own frame caps are the bound, and every one of them is named when it fires.
    InOutHold = UCk_LoadingProcess_Task_UE::Create(InWorldContext, InReason, FCk_Time{});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRelease_LoadScreenHold(
        TObjectPtr<UCk_LoadingProcess_Task_UE>& InOutHold) const
    -> void
{
    // Create returns null on a dedicated server, so an unset holder is a normal state rather than a missed one.
    if (ck::IsValid(InOutHold))
    { InOutHold->Request_Unregister(); }

    InOutHold = nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoAcquire_LoadStateChannel(
        UWorld& InWorld,
        bool InIsClientHold)
    -> void
{
    // ActorRelay channels are POOLED and shared with live consumers: the API is acquire-only by design and there
    // is nothing to release. The channel entity is what makes the fact reach a client at all — a replicated
    // container only rides an entity the replication driver knows about.
    auto Pending = UCk_Utils_ActorRelay_UE::Request_AcquireChannel(&InWorld,
        UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.Generic")));

    if (NOT UCk_Utils_PendingActorRelay_UE::Get_IsValid(Pending))
    {
        ck::snapshot::Warning(TEXT("Could not acquire an ActorRelay channel on world [{}] for the load's "
            "ready-to-resume fact — a client will not learn when this load finished and will release on its own "
            "bounded escape instead"), InWorld.GetName());
        return;
    }

    UCk_Utils_PendingActorRelay_UE::Promise_OnAcquired(Pending,
        [WeakThis = TWeakObjectPtr<UCk_Snapshot_Subsystem_UE>{this}, InIsClientHold]
        (FCk_ActorRelay_ChannelResult InResult) -> void
        {
            auto* Self = WeakThis.Get();
            if (ck::Is_NOT_Valid(Self))
            { return; }

            // Resolved through the subsystem rather than a captured reference: the promise can land a frame or
            // more after the call that armed it, and a raw reference into a member outlives nothing usefully.
            if (InIsClientHold)
            {
                Self->_ClientLoadStateChannelEntity = InResult.Get_ChannelEntity();
                return;
            }

            Self->_LoadStateChannelEntity = InResult.Get_ChannelEntity();

            // Converge from ARBITRARY state: this promise can land at any point in the load, including after
            // ready-to-resume has already been published to a channel that did not exist yet. Re-publishing the
            // CURRENT fact means a late channel carries the truth rather than the value the load happened to hold
            // when it was asked for — otherwise a client waiting on a slow channel releases on its bounded escape
            // for a load that finished long ago.
            Self->DoPublish_LoadState(Self->_IsReadyToResume);
        });

    // Kept so the pending handle outlives this scope while the channel is still coming up.
    if (InIsClientHold)
    { _PendingClientLoadStateChannel = Pending; }
    else
    { _PendingLoadStateChannel = Pending; }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoPublish_LoadState(
        bool InReadyToResume)
    -> void
{
    if (ck::Is_NOT_Valid(_LoadStateChannelEntity))
    {
        // Never silently — but the two publishes are not the same event. Publishing FALSE happens the moment the
        // channel is asked for, before its promise can have landed, and the acquired-callback re-publishes the
        // current fact when it does; that one is expected and routine. Publishing TRUE is the terminal outcome of
        // the client contract, and if it does not happen the only other evidence is a warning on a DIFFERENT
        // machine's log 600 frames later.
        if (InReadyToResume)
        {
            ck::snapshot::Warning(TEXT("Could not publish the load's READY-TO-RESUME fact for epoch [{}]: the "
                "ActorRelay channel entity is not resolved. A client following this load will not learn it "
                "finished and will release on its own bounded escape instead"), _LoadEpoch);
        }
        else
        {
            ck::snapshot::Verbose(TEXT("Load-state fact [{}] for epoch [{}] not published yet: the ActorRelay "
                "channel is still coming up. Its acquired-callback re-publishes whatever is current by then"),
                InReadyToResume, _LoadEpoch);
        }
        return;
    }

    auto Data = FCk_RepData_SnapshotLoadState{};
    Data.LoadEpoch = _LoadEpoch;
    Data.ReadyToResume = InReadyToResume;

    // Add-then-update rather than either alone: the first publish of a load has nothing to update, and every one
    // after it has nothing to add. Both are host-gated and no-op off the authority.
    UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_SnapshotLoadState>(_LoadStateChannelEntity, Data);
    UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_SnapshotLoadState>(_LoadStateChannelEntity, Data);

    // The server reads the fact the same way a client does, off the same fragment, so a listen server's own
    // release path is not a second implementation of the same question.
    auto& State = _LoadStateChannelEntity.AddOrGet<ck::FFragment_Snapshot_LoadState>();
    State.Set_LoadEpoch(_LoadEpoch);
    State.Set_ReadyToResume(InReadyToResume);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoGet_ShouldHoldWorldAtBoot(
        UWorld& InWorld)
    -> ECk_EcsWorld_LoadHold
{
    // The AUTHORITY's own post-travel world. Held from its first frame — the frame every level actor's BeginPlay
    // and every entity-script construction runs in — because the loader's own next opportunity is a full world
    // tick later, and by then the thing the hold exists to prevent has already happened.
    if (_LoadInProgress && DoIs_WorldOwnedByThisLoad(InWorld))
    {
        // Global time dilation is transient on a per-level AWorldSettings whose constructor writes 1.0, so the
        // freeze does NOT survive travel and this is where the post-travel world gets its own.
        DoApply_TimeFreeze(InWorld);

        // REBUILDING, not Converging. This world is about to run every level actor's BeginPlay and every
        // entity-script construction in it, and those are exactly the frames a level-triggered producer would
        // seed into an empty world beside the copies the loader is about to restore. Converging is deliberately
        // outside the spawn-suppression set (payload applies must be able to compose), so seeding it here would
        // leave the very window the seed was added for unsuppressed.
        return ECk_EcsWorld_LoadHold::Rebuilding;
    }

    // Which machine does this world belong to? Answered from THIS GameInstance's own load state, never from the
    // world's net role. UWorld::InternalGetNetMode falls back to AttemptDeriveFromURL and then to
    // PlayInEditorNetMode whenever the net driver is not yet attached (World.cpp:9461-9486), so "am I a client"
    // is not reliably answerable at OnWorldBeginPlay — which is the one instant this has to be answered, before
    // anything in the world ticks.
    //
    // Ownership is answerable, though, and it is the question that actually matters: a world carrying a load
    // epoch this GameInstance did not itself produce belongs to SOMEONE ELSE's load, and that is precisely what
    // makes this machine a client of it. The loader holds _LoadInProgress across its whole load and stamps its
    // own _LoadEpoch, so the authority refuses here on both counts and needs no role check to do it.
    if (const auto* EpochOption = InWorld.URL.GetOption(TEXT("CkLoad="), nullptr);
        EpochOption != nullptr)
    {
        // Validated, not merely present. FURL options survive a relative travel, and Atoi answers 0 for anything
        // malformed — while _LoadEpoch starts at 1, so an unvalidated read would arm a hold on an epoch no
        // publish can ever match and burn the whole budget failing open, indistinguishably from a real fault.
        const auto EpochString = FString{EpochOption};
        const auto Epoch = FCString::Atoi(EpochOption);

        if (NOT EpochString.IsNumeric() || Epoch <= 0)
        {
            ck::snapshot::Verbose(TEXT("Ignoring a ?CkLoad option this world cannot use: value [{}] is not a "
                "positive load epoch. No client hold is armed"), EpochString);
            return ECk_EcsWorld_LoadHold::None;
        }

        const auto ThisInstanceOwnsTheLoad = _LoadInProgress || Epoch == _LoadEpoch;

        if (NOT ThisInstanceOwnsTheLoad)
        {
            // The hold is per-EPOCH; the channel it releases on is per-WORLD. A client travels through more than
            // one world for a single load, and the channel acquired in an earlier one dies with it — leaving the
            // ticker watching a handle the server's fact can never reach. Measured exactly that way: the server
            // published READY TO RESUME and the client still burned its whole budget reporting NEVER ARRIVED.
            if (_ClientHoldActive && Epoch == _ClientHoldEpoch && _ClientHoldWorld.Get() != &InWorld)
            {
                DoRebind_ClientHold(InWorld);
                return ECk_EcsWorld_LoadHold::Converging;
            }

            // CONVERGING, not Rebuilding. A client rebuilds nothing — it has no saved rows and no loader — so the
            // kernel-only scope would hold its whole world out of its own replication-driven composition. What it
            // owes is coherence, which is exactly what Converging names.
            //
            // Reached on WHICHEVER world comes up first carrying the option, so a client that never passes
            // through a transition world still arms here rather than silently going unheld.
            if (NOT _ClientHoldActive)
            {
                DoBegin_ClientHold(InWorld, Epoch);
                return ECk_EcsWorld_LoadHold::Converging;
            }
        }
    }

    return ECk_EcsWorld_LoadHold::None;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoBegin_ClientHold(
        UWorld& InWorld,
        int32 InEpoch)
    -> void
{
    // A second reload while the first is still held: release the old one rather than stacking, or its ticker and
    // its screen holder outlive the world they belong to.
    if (_ClientHoldActive)
    { DoRelease_ClientHold(TEXT("a newer load's travel superseded it")); }

    _ClientHoldActive = true;
    _ClientHoldEpoch = InEpoch;
    _ClientHoldFrameCount = 0;
    _ClientHoldWorld = &InWorld;
    _ClientLoadStateChannelEntity = FCk_Handle{};
    _PendingClientLoadStateChannel = FCk_Handle_PendingActorRelay{};

    // The dilation the client writes here is a PREDICTION, and the server owns the value. AWorldSettings::
    // TimeDilation is replicated, so the server's floor — and later its restore — is what this world converges
    // on; the local write only closes the frame-0 gap before the first replication of the fresh world's settings
    // arrives, and the local restore closes the same gap on the way out. Both are superseded, never authoritative.
    DoApply_TimeFreeze(InWorld);

    DoCreate_LoadScreenHold(&InWorld, TEXT("CkSnapshot: the server is loading (not yet ready to resume)"),
        _ClientLoadScreenHold);

    DoAcquire_LoadStateChannel(InWorld, true);

    _ClientHoldTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_Snapshot_Subsystem_UE::DoTick_ClientHold));

    ck::snapshot::Display(TEXT("DIAG: client hold armed from the travel URL (epoch [{}], world [{}])"),
        InEpoch, InWorld.GetName());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRebind_ClientHold(
        UWorld& InWorld)
    -> void
{
    _ClientHoldWorld = &InWorld;

    // A fresh budget for the world that can actually receive the fact. The previous world's frames were spent
    // watching a channel that no longer exists, and charging them against this world would hand the player back
    // a world the client never really waited for. The CAP itself is untouched.
    _ClientHoldFrameCount = 0;

    // Dropped before re-acquiring: these name the dead world's channel, and a stale handle here is precisely
    // what made the release conjunct unreachable.
    _ClientLoadStateChannelEntity = FCk_Handle{};
    _PendingClientLoadStateChannel = FCk_Handle_PendingActorRelay{};

    // Dilation does not survive travel, so the prediction is re-applied per world exactly as the server re-applies
    // its own. Still a prediction: the server's replicated value supersedes it.
    DoApply_TimeFreeze(InWorld);

    DoAcquire_LoadStateChannel(InWorld, true);

    // The screen holder is GameInstance-scoped and still held, and the ticker is still registered — rebinding is
    // about WHICH world the hold watches, not about starting a second hold.
    ck::snapshot::Display(TEXT("DIAG: client hold re-bound to world [{}] (epoch [{}]) — re-acquiring the "
        "load-state channel on the world that will receive the fact"), InWorld.GetName(), _ClientHoldEpoch);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoTick_ClientHold(
        float /*InDeltaSeconds*/)
    -> bool
{
    if (NOT _ClientHoldActive)
    { return false; }

    ++_ClientHoldFrameCount;

    // BOTH conjuncts, and neither alone is enough. The fact says the server finished; replication-complete says
    // this client has actually received what that load produced. Releasing on the fact alone hands the player a
    // world whose contents are still arriving.
    const auto ChannelIsLive = ck::IsValid(_ClientLoadStateChannelEntity);
    const auto HasState = ChannelIsLive && _ClientLoadStateChannelEntity.Has<ck::FFragment_Snapshot_LoadState>();

    const auto FactHasArrived = HasState
        && _ClientLoadStateChannelEntity.Get<ck::FFragment_Snapshot_LoadState>().Get_ReadyToResume()
        && _ClientLoadStateChannelEntity.Get<ck::FFragment_Snapshot_LoadState>().Get_LoadEpoch() == _ClientHoldEpoch;

    const auto ReplicationIsComplete = ChannelIsLive
        && UCk_Utils_EntityReplicationDriver_UE::Get_IsReplicationCompleteAllDependents(_ClientLoadStateChannelEntity);

    if (FactHasArrived && ReplicationIsComplete)
    {
#if WITH_AUTOMATION_TESTS
        _TestOnly_ClientHoldReleasedByCap = false;
#endif
        DoRelease_ClientHold(TEXT("the server reported ready-to-resume and this client's replication completed"));
        return false;
    }

    if (_ClientHoldFrameCount < kLoad_ClientHoldFrameCap)
    { return true; }

    // Tenet 7 again, and the escape NAMES which conjunct never arrived — "the client hold timed out" is a
    // symptom, "the fact never replicated" and "the channel never completed" are two different bugs.
    ck::snapshot::Warning(TEXT("Client hold for load epoch [{}] hit its [{}]-frame cap and is releasing: the "
        "ready-to-resume fact [{}] and this client's channel replication [{}]. The world is being handed back "
        "anyway — a client held forever is a client that cannot play"),
        _ClientHoldEpoch, kLoad_ClientHoldFrameCap,
        FactHasArrived ? TEXT("arrived") : TEXT("NEVER ARRIVED"),
        ReplicationIsComplete ? TEXT("completed") : TEXT("NEVER COMPLETED"));

#if WITH_AUTOMATION_TESTS
    _TestOnly_ClientHoldReleasedByCap = true;
#endif

    DoRelease_ClientHold(TEXT("the client hold hit its frame cap"));
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRelease_ClientHold(
        const TCHAR* InReason)
    -> void
{
    if (_ClientHoldTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_ClientHoldTickerHandle);
        _ClientHoldTickerHandle.Reset();
    }

    if (NOT _ClientHoldActive)
    { return; }

    if (auto* HeldWorld = _ClientHoldWorld.Get();
        HeldWorld != nullptr)
    {
        if (auto* EcsWorld = HeldWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
            ck::IsValid(EcsWorld))
        { EcsWorld->Set_LoadHold(ECk_EcsWorld_LoadHold::None); }

        DoRestore_TimeFreeze(*HeldWorld);
    }

    DoRelease_LoadScreenHold(_ClientLoadScreenHold);

    _ClientHoldActive = false;
    _ClientHoldWorld = nullptr;
    _ClientLoadStateChannelEntity = FCk_Handle{};
    _PendingClientLoadStateChannel = FCk_Handle_PendingActorRelay{};

    ck::snapshot::Display(TEXT("DIAG: client hold released for epoch [{}] — {}"), _ClientHoldEpoch, InReason);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoReconcile_Queue()
    -> void
{
    auto SavedChildLabelsByOwnerId = TMap<uint32, TSet<FString>>{};
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        if (Entry.Get_Provenance() != ECk_Snapshot_V3_Provenance::ConstructSpawned)
        { continue; }
        SavedChildLabelsByOwnerId.FindOrAdd(Entry.Get_LifetimeOwnerSavedId()).Add(Entry.Get_Label());
    }

    const auto SaveTypes = FCk_PersistenceHandlerRegistry::Get_SaveHandlerTypes();
    const auto DoAnyProduce = [&](FCk_Handle& InEntity) -> bool
    {
        for (const auto* Type : SaveTypes)
        {
            const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Type);
            if (Handler == nullptr || NOT Handler->Produce)
            { continue; }
            if (Handler->Produce(InEntity).IsSet())
            { return true; }
        }
        return false;
    };

    auto DestroyedCount = 0;
    for (const auto& Pair : _SavedIdMap)
    {
        const auto OwnerSavedId = Pair.Key;
        auto Owner = Pair.Value;
        if (ck::Is_NOT_Valid(Owner) || NOT Owner.Has<ck::FFragment_LifetimeDependents>())
        { continue; }

        const auto* SavedLabels = SavedChildLabelsByOwnerId.Find(OwnerSavedId);

        // Copy — Request_DestroyEntity mutates the dependents list.
        auto Children = Owner.Get<ck::FFragment_LifetimeDependents>().Get_Entities();
        for (auto& Child : Children)
        {
            if (ck::Is_NOT_Valid(Child) || NOT Child.Has<ck::FTag_ConstructSpawned>())
            { continue; }
            // Save-transient children are payload-persisted derived state (attributes, SM graph, ...) — never
            // captured as rows, so "absent from the save" is their NORMAL state, not a revoked grant.
            if (Child.Has<ck::FTag_Snapshot_SaveTransient>())
            { continue; }
            // Reconstruct-only children are intentionally absent from the save. Their feature recreates them from
            // authored defaults after the load boundary, so absence is not a revoked grant and must not trigger
            // subtractive reconciliation.
            if (Child.Has<ck::FTag_Snapshot_ReconstructOnly>())
            { continue; }
            if (NOT UCk_Utils_GameplayLabel_UE::Has(Child) || UCk_Utils_GameplayLabel_UE::Get_IsUnnamedLabel(Child))
            { continue; }

            const auto ChildLabel = UCk_Utils_GameplayLabel_UE::Get_Label(Child).ToString();
            const auto bSaved = SavedLabels != nullptr && SavedLabels->Contains(ChildLabel);
            if (NOT bSaved)
            {
                // Payload-bearing children are feature STATE, not grants — the capture may have skipped them
                // (composed post-construct in the save world) even though this world composed them in-construct.
                // Subtracting them would destroy live feature state the save cannot express.
                if (DoAnyProduce(Child))
                {
                    ck::snapshot::Verbose(TEXT("v3 reconcile: keeping payload-bearing ConstructSpawned child [{}] "
                        "label [{}] of owner [{}] — absent from the save but carries feature state"),
                        Child, ChildLabel, Owner);
                    continue;
                }
                // A labeled child rebuilt by Construct but ABSENT from the save is a grant the player lost. The
                // deferred teardown PARKS under the gate and completes post-gate; we only queue here, never wait.
                ck::snapshot::Verbose(TEXT("v3 reconcile: destroying stray ConstructSpawned child [{}] label [{}] "
                    "of owner [{}] (saved-id [{}]) — absent from the save"),
                    Child, ChildLabel, Owner, OwnerSavedId);
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Child);
                ++DestroyedCount;
            }
        }
    }

    if (DestroyedCount > 0)
    { ck::snapshot::Display(TEXT("DIAG: v3 reconcile — queued [{}] stray ConstructSpawned children for destruction"), DestroyedCount); }
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
                return true;
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

            ck::snapshot::Display(TEXT("DIAG: post-travel world ready after [{}] frames — activating load gate + rebuilding"), _LoadFrameCount);

            // Freeze feature processors against the half-rebuilt world. The kernel (EntityScript pipeline,
            // hydration dispatcher, ...) keeps running so the rebuild + construction can proceed.
            DoSet_LoadHold(ECk_EcsWorld_LoadHold::Rebuilding);

            if (auto* EcsWorld = DoGet_LoadWorldEcs();
                ck::IsValid(EcsWorld))
            {
                // Zero the apply tally HERE and not in Request_Load: the counters live in the registry, and the
                // registry Request_Load ran against belongs to the world this load just travelled away from.
                // Explicit assignment because the context API is get-or-create — an emplace onto an existing
                // context returns it untouched, which would carry the previous load's counts into this one.
                EcsWorld->Get_Registry().SetContext<ck::FCtx_HydrationOutcomes>() = ck::FCtx_HydrationOutcomes{};
                EcsWorld->Get_Registry().SetContext<ck::FCtx_LoadConvergence>() = ck::FCtx_LoadConvergence{};

                // The channel was acquired on the world this load travelled AWAY from, so its entity died with
                // it. Re-acquire against the world the fact now has to be published from.
                _LoadStateChannelEntity = FCk_Handle{};
                _PendingLoadStateChannel = FCk_Handle_PendingActorRelay{};
                if (auto* PostTravelWorld = GetWorld();
                    ck::IsValid(PostTravelWorld))
                { DoAcquire_LoadStateChannel(*PostTravelWorld, false); }
                DoPublish_LoadState(false);
            }

            ck::snapshot::Display(TEXT("DIAG: rehydrated SaveKey resolver with [{}] live entries"),
                DoRehydrate_SaveKeyResolver());

            _LoadFrameCount = 0;
            _LoadPhase = ELoadPhase::Rebuilding;
            return true;
        }

        case ELoadPhase::Rebuilding:
        {
            const auto Complete = DoRebuild_Tick();

            // Some saved entities may never resolve (content drift, infra the fresh world owns). Rather than always
            // burn kLoad_RebuildFrameCap, proceed once no NEW entity maps for kLoad_RebuildStallTicks ticks.
            if (_SavedIdMap.Num() > _RebuildLastMappedCount)
            { _RebuildLastMappedCount = _SavedIdMap.Num(); _RebuildStallTicks = 0; }
            else
            { ++_RebuildStallTicks; }

            const auto Stalled = _RebuildStallTicks >= kLoad_RebuildStallTicks;

            if (NOT Complete && NOT Stalled && _LoadFrameCount < kLoad_RebuildFrameCap)
            { return true; } // keep polling

            if (NOT Complete && NOT _RebuildEscalated)
            {
                // The kernel quiesced with rows still unresolved. A row can legitimately depend on work the
                // kernel cannot run: a multi-stage construction (EntityScript `Continue`) is finished by a
                // GAME processor, and the identity it stamps on completion — a child's GameplayLabel adopt
                // key, a SaveKey — is exactly what the resolution scan is waiting on (the child-adopt
                // orphan incident, 2026-07-29). Escalate to full-scope ticks and only conclude when THAT
                // quiesces too; orphaning here would silently drop every payload under the waiting rows.
                _RebuildEscalated = true;
                DoSet_LoadHold(ECk_EcsWorld_LoadHold::Escalated);
                _RebuildStallTicks = 0;
                _LoadFrameCount = 0;
                _V3LoadReport.Set_UsedEscalatedRebuild(true);
                ck::snapshot::Display(TEXT("DIAG: rebuild kernel quiesced with [{}]/[{}] mapped — escalating to zero-time full-scope ticks"),
                    _SavedIdMap.Num(), _V3Tables.Get_Entities().Num());
                return true;
            }

            if (NOT Complete)
            {
                auto UnEngine = 0, UnConstruct = 0, UnRuntime = 0, UnDefinitionBuilt = 0;
                for (const auto& Entry : _V3Tables.Get_Entities())
                {
                    if (_SavedIdMap.Contains(Entry.Get_SavedId()) || _SkippedIds.Contains(Entry.Get_SavedId()))
                    { continue; }
                    switch (Entry.Get_Provenance())
                    {
                        case ECk_Snapshot_V3_Provenance::EngineOwned:      ++UnEngine; break;
                        case ECk_Snapshot_V3_Provenance::ConstructSpawned: ++UnConstruct; break;
                        case ECk_Snapshot_V3_Provenance::RuntimeSpawned:   ++UnRuntime; break;
                        case ECk_Snapshot_V3_Provenance::DefinitionBuilt:  ++UnDefinitionBuilt; break;
                    }
                    ck::snapshot::Verbose(TEXT("DIAG: unresolved v3 entry saved-id [{}] provenance [{}] owner-saved-id [{}] "
                        "(owner mapped: [{}]) label [{}] scriptClass [{}] actorClass [{}]"),
                        Entry.Get_SavedId(), static_cast<int32>(Entry.Get_Provenance()), Entry.Get_LifetimeOwnerSavedId(),
                        _SavedIdMap.Contains(Entry.Get_LifetimeOwnerSavedId()), Entry.Get_Label(),
                        Entry.Get_ScriptClassPath(), Entry.Get_ActorClassPath());
                }
                _V3LoadReport.Set_UnresolvedAfterEscalation(UnEngine + UnConstruct + UnRuntime + UnDefinitionBuilt);
                ck::snapshot::Error(TEXT("Request_Load: ESCALATED rebuild {} — [{}]/[{}] mapped; unresolved by provenance: "
                    "EngineOwned [{}], ConstructSpawned [{}], RuntimeSpawned [{}], DefinitionBuilt [{}]. The full processor "
                    "scope quiesced and these rows still cannot resolve (content drift, or an identity the fresh world "
                    "never re-creates) — every payload under them will be recorded as orphaned. Proceeding (partial load)."),
                    Stalled ? TEXT("stalled (no progress)") : TEXT("hit frame cap"),
                    _SavedIdMap.Num(), _V3Tables.Get_Entities().Num(), UnEngine, UnConstruct, UnRuntime, UnDefinitionBuilt);
            }

            ck::snapshot::Display(TEXT("DIAG: rebuild complete after [{}] frames — [{}]/[{}] entities mapped ([{}] skipped), hydrating"),
                _LoadFrameCount, _SavedIdMap.Num(), _V3Tables.Get_Entities().Num(), _SkippedIds.Num());
            _LoadFrameCount = 0;
            _LoadPhase = ELoadPhase::Hydrating;
            return true;
        }

        case ELoadPhase::Hydrating:
        {
            DoHydrate_Enqueue();
            DoReconcile_Queue();

            // The scope opens to FULL, but the hold does NOT come off: the payloads, the deferred requests those
            // applies issue, and the parked reconcile-destroys all need the whole processor set to run — and none
            // of it should be PACED, because the player is still looking at a loading screen.
            DoSet_LoadHold(ECk_EcsWorld_LoadHold::Draining);

            if (auto* EcsWorld = DoGet_LoadWorldEcs();
                ck::IsValid(EcsWorld))
            { EcsWorld->Request_PumpToQuiescence(ck::ECk_SchedulerTickScope::Full); }

            _SettleFramesRemaining = kLoad_SettleFrames;
            _SettleStarted = true;
            _LoadFrameCount = 0;
            _LoadPhase = ELoadPhase::Draining;
            return true;
        }

        case ELoadPhase::Draining:
        {
            // Finish only once hydration has fully drained AND the parked reconcile-destroys have had their minimum
            // settle frames. The frame cap is a LOUD abort backstop — reaching it means some payloads never applied.
            if (_SettleFramesRemaining > 0)
            { --_SettleFramesRemaining; }

            // The whole mapped set comes off quarantine together the moment the payload queue is empty — never one
            // entity at a time, so a released entity can never read a still-quarantined sibling's fragment. Then at
            // least one more FULL pump runs before the load finishes, which is the pass the released Setups take.
            if (_QuarantineStamped && DoIs_PayloadDrainComplete())
            {
                DoLift_HydrationQuarantine(EQuarantineLift::Settled, _V3LoadReport);
                _SettleFramesRemaining = FMath::Max(_SettleFramesRemaining, 1);
                return true;
            }

            const auto FrameCap = DoGet_HydrateFrameCap();
            const auto HydrationPending = NOT DoIs_HydrationComplete();
            if ((HydrationPending || _SettleFramesRemaining > 0) && _LoadFrameCount < FrameCap)
            { return true; }

            if (HydrationPending)
            {
                CK_TRIGGER_ENSURE(TEXT("Request_Load: settle hit the [{}]-frame cap with hydration still pending — "
                    "finishing anyway (some payloads did not apply)"), FrameCap);

                // Fail-closed needs an escape: entities held for a queue that never drained are released here and
                // each names itself in the report, rather than staying invisible to every processor for the session.
                DoLift_HydrationQuarantine(EQuarantineLift::ForcedAtFrameCap, _V3LoadReport);
            }

            // The old breadcrumb, re-worded: it no longer means the load is finishing, because a convergence
            // phase follows it. The ready-to-resume line is the one a consumer should watch.
            ck::snapshot::Display(TEXT("DIAG: v3 load payloads drained — converging (restored [{}], orphaned [{}])"),
                _V3LoadReport.Get_EntitiesRestored(), _V3LoadReport.Get_EntitiesOrphaned());

            DoSet_LoadHold(ECk_EcsWorld_LoadHold::Converging);

            if (auto* EcsWorld = DoGet_LoadWorldEcs();
                ck::IsValid(EcsWorld))
            {
                // Zeroed at the phase's entry, never at Request_Load: the counters live in the registry, and the
                // registry Request_Load ran against belongs to the world this load travelled away from. Explicit
                // assignment because the context API is get-or-create. Each module's own baseline is stamped by
                // its own advance on the first frame, so nothing here has to know what those baselines are.
                EcsWorld->Get_Registry().SetContext<ck::FCtx_LoadConvergence>() = ck::FCtx_LoadConvergence{};
            }

            _ConvergenceFramesSatisfied = 0;
            _ConvergencePendingLastFrame.Reset();
            _ConvergencePumpSeries.Reset();
            _ConvergenceSkippedSeries.Reset();
            _LoadFrameCount = 0;
            _LoadPhase = ELoadPhase::Converging;
            return true;
        }

        case ELoadPhase::Converging:
        {
            // Drive FIRST, then read. The physics grant and the pump are deliberate once-per-frame ACTIONS; the
            // predicates that judge the result are pure reads of what those actions produced, which is what keeps
            // a convergence check from certifying its own side effects.
            DoDrive_Convergence();

            auto* EcsWorld = DoGet_LoadWorldEcs();
            const auto Pending = ck::IsValid(EcsWorld)
                ? ck::FCk_LoadConvergenceRegistry::Get_Pending(EcsWorld->Get_Registry())
                : TArray<FName>{};

            DoReport_ConvergenceProgress(Pending);

            if (NOT Pending.IsEmpty() && _LoadFrameCount >= kLoad_ConvergenceDebugArmFrame)
            { DoArm_ConvergenceDebugTiming(); }

            if (Pending.IsEmpty())
            { ++_ConvergenceFramesSatisfied; }
            else
            { _ConvergenceFramesSatisfied = 0; }

            // Consecutive frames, not one. A single frame can be a fact that has not started rather than one that
            // has finished — the first frame of the phase is exactly that for anything the grant is about to move.
            if (_ConvergenceFramesSatisfied >= kLoad_ConvergenceQuiescentFrames)
            {
                ck::snapshot::Display(TEXT("DIAG: v3 converged after [{}] frames — pump counts last [{}]: [{}], "
                    "skipped last [{}]: [{}], granted physics steps [{}]"),
                    _LoadFrameCount, _ConvergencePumpSeries.Num(),
                    DoGet_ConvergenceSeriesText(_ConvergencePumpSeries), _ConvergenceSkippedSeries.Num(),
                    DoGet_ConvergenceSeriesText(_ConvergenceSkippedSeries), DoGet_ConvergenceGrantedSteps());

                DoEnter_ReadyToResume();
                return false; // done — unregister
            }

            if (_LoadFrameCount < DoGet_ConvergenceFrameCap())
            { return true; }

            // Beside the per-name Errors below, one line that says what the phase was still DOING while those
            // facts refused to settle. Without it a cap hit is a name and nothing else, and the difference
            // between "a fact nobody drives" and "a pump that never went quiet" needs a repro to tell apart.
            ck::snapshot::Display(TEXT("DIAG: v3 convergence hit the [{}]-frame cap — pump counts last [{}]: [{}], "
                "skipped groups last [{}]: [{}], granted physics steps [{}], still pending [{}]"),
                DoGet_ConvergenceFrameCap(), _ConvergencePumpSeries.Num(),
                DoGet_ConvergenceSeriesText(_ConvergencePumpSeries), _ConvergenceSkippedSeries.Num(),
                DoGet_ConvergenceSeriesText(_ConvergenceSkippedSeries), DoGet_ConvergenceGrantedSteps(),
                Pending.Num());

            // Tenet 7: fail-closed needs a bounded escape, and the escape has to NAME what it gave up on. Each
            // remaining fact becomes an Error and a record, the Result downgrades to Succeeded_WithLoss, and the
            // world is handed back — a world resumed early is recoverable, a world never handed back is not.
            DoReport_ConvergenceStall();
            DoRecord_ConvergenceUnmet(_V3LoadReport);
            DoEnter_ReadyToResume();
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
    // Every route into here releases everything the load put in place. The healthy one has already done it in
    // DoEnter_ReadyToResume — this is idempotent — but the teardown and travel aborts reach here directly, and a
    // world left held, frozen or behind a loading screen is a wedge nothing else will clear.
    DoSet_LoadHold(ECk_EcsWorld_LoadHold::None);

    if (auto* FrozenWorld = _TimeFreezeWorld.Get();
        FrozenWorld != nullptr)
    { DoRestore_TimeFreeze(*FrozenWorld); }

    DoRelease_LoadScreenHold(_LoadScreenHold);
    DoRestore_ConvergenceDebugTiming();

    // Frozen FIRST, then completed in place. Two of the three routes into here build their report LOCALLY, so a
    // lift or a fold that wrote to _V3LoadReport would name what it found in a copy nobody reads; and every
    // consumer — the pull channel, the signal, the delegate — is handed this one object.
    _LastLoadReport = InReport;

    // The unconditional escape, covering every route into here, the abort paths included.
    DoLift_HydrationQuarantine(EQuarantineLift::ForcedAtLoadFinish, _LastLoadReport);

    // Read the apply tally ONCE, here, and sweep whatever is still queued. Anything the dispatcher does after
    // this point is logged rather than counted: the report describes the load, not the queue's whole life.
    DoFold_HydrationOutcomes(_LastLoadReport);

    // LAST, and after the fold: the verdict is a statement about the buckets, so it cannot be computed before they
    // are filled. Computing it here rather than at each call site is also what keeps DoFinish_Load's signature.
    DoCompute_LoadResult(_LastLoadReport);

    const auto AccountingIsClosed = _LastLoadReport.Get_IsAccountingClosed();
    CK_ENSURE_IF_NOT(AccountingIsClosed,
        TEXT("v3 load accounting does not close: entities [{}] vs restored [{}] + skipped [{}] + orphaned [{}]; ")
        TEXT("payloads [{}] vs applied [{}] + rejected [{}] + no-handler [{}] + timed-out [{}] + destroyed [{}] + ")
        TEXT("unapplied-at-finish [{}] + on-skipped [{}] + on-orphaned [{}] + unresolved-owner [{}] + dropped [{}]"),
        _LastLoadReport.Get_EntitiesTotal(), _LastLoadReport.Get_EntitiesRestored(),
        _LastLoadReport.Get_EntitiesSkipped(), _LastLoadReport.Get_EntitiesOrphaned(),
        _LastLoadReport.Get_PayloadsTotal(), _LastLoadReport.Get_PayloadsApplied(),
        _LastLoadReport.Get_PayloadsRejected(), _LastLoadReport.Get_PayloadsDroppedNoHandler(),
        _LastLoadReport.Get_PayloadsDroppedTimeout(), _LastLoadReport.Get_PayloadsDestroyedWithEntries(),
        _LastLoadReport.Get_PayloadsUnappliedAtFinish(), _LastLoadReport.Get_PayloadsOnSkippedEntities(),
        _LastLoadReport.Get_PayloadsOnOrphanedEntities(), _LastLoadReport.Get_PayloadsOnUnresolvedOwner(),
        _LastLoadReport.Get_PayloadsDropped())
    {}

    _LoadTickerHandle.Reset(); // DoTick_Load returns false to unregister; just drop our copy of the handle
    _LoadPhase = ELoadPhase::Idle;
    _LoadInProgress = false;
    _ConvergenceFramesSatisfied = 0;
    _ConvergencePendingLastFrame.Reset();
    _ConvergencePumpSeries.Reset();
    _ConvergenceSkippedSeries.Reset();
    _PendingTeardownRoots.Reset();
    _V3Tables = FCk_Snapshot_V3_Tables{};
    _SavedIdMap.Reset();
    _MappedLiveEntities.Reset();
    _SpawnedRuntimeIds.Reset();
    _RuntimeEntityScriptsAwaitingConstruction.Reset();
    _SkippedIds.Reset();
    _SkipRecords.Reset();
    _PersistedIds.Reset();
    _PendingBridgeActors.Reset();

    const auto Delegate = _PendingLoadDelegate;
    _PendingLoadDelegate.Unbind();

    // Swapped out BEFORE draining, so the list being iterated is not the list a callback can add to. _LoadInProgress
    // is already false by here, so a promise re-armed from inside a callback takes the immediate path and fires
    // there and then — landing it in this array would either be dropped or delivered twice.
    const auto Promises = MoveTemp(_PendingLoadCompletePromises);
    _PendingLoadCompletePromises.Reset();

    const auto Source = DoGet_SnapshotSource(); // re-resolve: the fresh world's transient
    ck::UUtils_Signal_Snapshot_OnLoadComplete::Broadcast(Source, ck::MakePayload(Source, _LastLoadReport));

    for (const auto& Promise : Promises)
    { Promise.ExecuteIfBound(Source, _LastLoadReport); }

    Delegate.ExecuteIfBound(_LastLoadReport);
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

TArray<FName>
    UCk_Snapshot_Subsystem_UE::
    Get_AllSaveSlotNames() const
{
    auto* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();

    if (ck::Is_NOT_Valid(SaveSystem, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto FoundSlots = TArray<FString>{};

    // Platforms that cannot enumerate return false; that is indistinguishable from "no saves" here,
    // which is why a fixed-slot menu should drive off its own slot names and use this only to
    // discover what exists.
    if (NOT SaveSystem->GetSaveGameNames(FoundSlots, ck_snapshot_subsystem::UserIndex))
    { return {}; }

    auto Result = TArray<FName>{};
    Result.Reserve(FoundSlots.Num());

    for (const auto& SlotName : FoundSlots)
    {
        if (ck::snapshot::slot_meta::Get_IsMetaSlotName(SlotName))
        { continue; }

        Result.Emplace(FName{*SlotName});
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Snapshot_SlotMeta
    UCk_Snapshot_Subsystem_UE::
    Get_SaveSlotMeta(
        FName InSlotName) const
{
    auto* MetaSaveGame = Cast<UCk_Snapshot_SlotMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(
        ck::snapshot::slot_meta::Get_MetaSlotName(InSlotName), ck_snapshot_subsystem::UserIndex));

    if (ck::Is_NOT_Valid(MetaSaveGame))
    { return {}; }

    return MetaSaveGame->_Meta;
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Snapshot_Subsystem_UE::
    Request_DeleteSaveSlot(
        FName InSlotName)
{
    const auto CanDelete = NOT _SnapshotInProgress && NOT _LoadInProgress;
    CK_ENSURE_IF_NOT(CanDelete,
        TEXT("Request_DeleteSaveSlot refused for slot [{}]: a snapshot operation is in progress"), InSlotName)
    { return false; }

    // Sidecar first: a snapshot with no sidecar lists as untitled, whereas a sidecar with no
    // snapshot would list a slot that cannot be loaded.
    if (UGameplayStatics::DoesSaveGameExist(ck::snapshot::slot_meta::Get_MetaSlotName(InSlotName), ck_snapshot_subsystem::UserIndex))
    { UGameplayStatics::DeleteGameInSlot(ck::snapshot::slot_meta::Get_MetaSlotName(InSlotName), ck_snapshot_subsystem::UserIndex); }

    if (NOT UGameplayStatics::DoesSaveGameExist(InSlotName.ToString(), ck_snapshot_subsystem::UserIndex))
    {
        ck::snapshot::Warning(TEXT("Request_DeleteSaveSlot: slot [{}] does not exist"), InSlotName);
        return false;
    }

    const auto Deleted = UGameplayStatics::DeleteGameInSlot(InSlotName.ToString(), ck_snapshot_subsystem::UserIndex);

    if (NOT Deleted)
    { ck::snapshot::Error(TEXT("Request_DeleteSaveSlot: DeleteGameInSlot failed for slot [{}]"), InSlotName); }
    else
    { ck::snapshot::Display(TEXT("Request_DeleteSaveSlot: deleted slot [{}]"), InSlotName); }

    return Deleted;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Snapshot_Header
    UCk_Snapshot_Subsystem_UE::
    Get_SaveSlotHeader(
        FName InSlotName) const
{
    auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(UGameplayStatics::LoadGameFromSlot(InSlotName.ToString(), ck_snapshot_subsystem::UserIndex));
    if (ck::Is_NOT_Valid(SaveGame) || SaveGame->_SnapshotBytesV3.Num() == 0)
    { return {}; } // invalid slot, or a pre-v3 slot with no v3 header of record

    // The SaveGame stores only the v3 header; this frozen BP return type is synthesized from the six overlapping
    // fields. The legacy-only stream field (manifest) has no v3 source and stays default.
    const auto& HeaderV3 = SaveGame->_HeaderV3;
    auto Header = FCk_Snapshot_Header{};
    Header.Set_FormatVersion(HeaderV3.Get_FormatVersion())
          .Set_EngineVersion(HeaderV3.Get_EngineVersion())
          .Set_PluginBuildHash(HeaderV3.Get_PluginBuildHash())
          .Set_TimestampUTC(HeaderV3.Get_TimestampUTC())
          .Set_WorldAssetPath(HeaderV3.Get_WorldAssetPath())
          .Set_EntityCount(HeaderV3.Get_EntityCount());
    return Header;
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

bool
    UCk_Snapshot_Subsystem_UE::
    TryPublish_SaveKey(
        FGuid InKey,
        FCk_Handle InHandle)
{
    return DoTryPublish_SaveKey(InKey, InHandle, true);
}

bool
    UCk_Snapshot_Subsystem_UE::
    DoTryPublish_SaveKey(
        FGuid InKey,
        FCk_Handle InHandle,
        bool InDiagnoseCollision)
{
    const auto KeyIsValid = InKey.IsValid();
    CK_ENSURE_IF_NOT(KeyIsValid,
        TEXT("Cannot publish an invalid Snapshot SaveKey for Entity [{}]"), InHandle)
    { return false; }

    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Cannot publish Snapshot SaveKey [{}] for an invalid Entity Handle"), InKey)
    { return false; }

    if (const auto* Existing = _SaveKeyResolverMap.Find(InKey))
    {
        if (*Existing == InHandle)
        { return true; }

        // The resolver can outlive a resolved entity. A stale mapping is not a
        // live collision, so replace it before querying fragments on the old
        // handle (which is invalid by definition).
        if (ck::Is_NOT_Valid(*Existing))
        {
            _SaveKeyResolverMap.Add(InKey, InHandle);
            return true;
        }

        // ActorRelay-style pools deliberately give every interchangeable
        // member one group key. Accept that cardinality only when BOTH live
        // publishers explicitly mark this exact canonical key as shared;
        // unique keys and mixed shared/unique collisions remain loud.
        const auto IsSharedPublisher = [InKey](const FCk_Handle& InPublisher) -> bool
        {
            if (NOT InPublisher.Has<FFragment_SaveKey>())
            { return false; }

            const auto& SaveKey = InPublisher.Get<FFragment_SaveKey>();
            return SaveKey.Get_Key() == InKey && SaveKey.Get_IsSharedRendezvousGroup();
        };
        if (IsSharedPublisher(*Existing) && IsSharedPublisher(InHandle))
        { return true; }

        if (InDiagnoseCollision)
        {
            CK_ENSURE_IF_NOT(false,
                TEXT("Snapshot SaveKey [{}] is already published by Entity [{}]; rejecting conflicting Entity [{}]"),
                InKey, *Existing, InHandle)
            {}
        }
        return false;
    }

    _SaveKeyResolverMap.Add(InKey, InHandle);
    return true;
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
// Offline slot census: what a save CONTAINS, without loading it. The duplicate (owner, label) adopt-key report is
// the signature of double-population — two live copies of one logical child captured under the same key, which the
// loader can then only bind positionally and the reconcile can never subtract (the label IS present in the save).

namespace ck_snapshot_subsystem
{
    static FAutoConsoleCommandWithWorldAndArgs GCmd_Snapshot_DumpSlot(
        TEXT("Ck.Snapshot.DumpSlot"),
        TEXT("Census a save slot without loading it: provenance totals, per-identity row counts, duplicate (owner,label) adopt-key groups. Usage: Ck.Snapshot.DumpSlot <SlotName>"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& InArgs, UWorld* /*InWorld*/) -> void
        {
            if (InArgs.IsEmpty())
            {
                ck::snapshot::Error(TEXT("Ck.Snapshot.DumpSlot: usage: Ck.Snapshot.DumpSlot <SlotName>"));
                return;
            }

            const auto& SlotName = InArgs[0];
            const auto Document = ck::snapshot::Inspect_SaveSlot(FName{*SlotName});

            if (Document.Get_ReadStatus() != ECk_SnapshotInspection_ReadStatus::Success)
            {
                ck::snapshot::Error(TEXT("Ck.Snapshot.DumpSlot: cannot census slot [{}] — read status [{}]"),
                    SlotName, ck::snapshot::Get_ReadStatusText(Document.Get_ReadStatus()));
                return;
            }

            auto IdentityCounts = TMap<FString, int32>{};
            auto AdoptKeyCounts = TMap<FString, int32>{};

            for (const auto& Summary : Document.Get_Entities())
            {
                const auto& Entry = Summary.Get_Entry();

                ++IdentityCounts.FindOrAdd(ck::Format_UE(TEXT("[{}] {}"),
                    ck::snapshot::Get_ProvenanceText(Entry.Get_Provenance()), Summary.Get_IdentityText()));

                if (Entry.Get_Provenance() == ECk_Snapshot_V3_Provenance::ConstructSpawned)
                {
                    ++AdoptKeyCounts.FindOrAdd(ck::Format_UE(TEXT("owner [{}] label [{}]"),
                        Entry.Get_LifetimeOwnerSavedId(), Entry.Get_Label()));
                }
            }

            const auto& Census = Document.Get_Census();
            ck::snapshot::Display(TEXT("DumpSlot [{}]: entities [{}] = EngineOwned [{}] + ConstructSpawned [{}] + "
                "RuntimeSpawned [{}] + DefinitionBuilt [{}] | payloads [{}]"),
                SlotName, Census.Get_EntityCount(), Census.Get_EngineOwnedCount(), Census.Get_ConstructSpawnedCount(),
                Census.Get_RuntimeSpawnedCount(), Census.Get_DefinitionBuiltCount(), Census.Get_PayloadCount());

            auto SortedIdentities = IdentityCounts.Array();
            SortedIdentities.Sort([](const auto& InA, const auto& InB) { return InA.Value > InB.Value; });

            constexpr auto MaxIdentityLines = 60;
            auto PrintedIdentities = 0;
            auto SingletonIdentities = 0;
            for (const auto& Kvp : SortedIdentities)
            {
                if (Kvp.Value < 2)
                { ++SingletonIdentities; continue; }
                if (PrintedIdentities < MaxIdentityLines)
                {
                    ck::snapshot::Display(TEXT("DumpSlot census: x{}  {}"), Kvp.Value, Kvp.Key);
                    ++PrintedIdentities;
                }
            }
            if (PrintedIdentities == MaxIdentityLines)
            { ck::snapshot::Display(TEXT("DumpSlot census: ... (list capped at [{}] lines)"), MaxIdentityLines); }
            ck::snapshot::Display(TEXT("DumpSlot census: plus [{}] singleton identities (count 1)"), SingletonIdentities);

            auto DuplicateKeys = AdoptKeyCounts.Array();
            DuplicateKeys.RemoveAll([](const auto& InKvp) { return InKvp.Value < 2; });
            DuplicateKeys.Sort([](const auto& InA, const auto& InB) { return InA.Value > InB.Value; });

            ck::snapshot::Display(TEXT("DumpSlot adopt-keys: [{}] duplicate (owner, label) groups"), DuplicateKeys.Num());
            constexpr auto MaxDuplicateLines = 40;
            for (auto Index = 0; Index < DuplicateKeys.Num() && Index < MaxDuplicateLines; ++Index)
            {
                ck::snapshot::Display(TEXT("DumpSlot adopt-keys: x{}  {}"),
                    DuplicateKeys[Index].Value, DuplicateKeys[Index].Key);
            }
            if (DuplicateKeys.Num() > MaxDuplicateLines)
            { ck::snapshot::Display(TEXT("DumpSlot adopt-keys: ... (list capped at [{}] lines)"), MaxDuplicateLines); }
        }));
}

// --------------------------------------------------------------------------------------------------------------------
