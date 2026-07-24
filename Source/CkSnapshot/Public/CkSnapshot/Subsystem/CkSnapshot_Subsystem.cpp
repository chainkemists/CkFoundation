#include "CkSnapshot_Subsystem.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/SaveGame/CkSnapshot_SaveGame.h"
#include "CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h" // v3 recipe+payload capture (the live save path)
#include "CkSnapshot/SaveKey/CkSnapshot_SaveKey_Fragment.h"   // EngineOwned rendezvous resolver
#include "CkSnapshot/Subsystem/CkSnapshot_Signals.h"

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

#include "CkEcsExt/Transform/CkTransform_Utils.h"             // G1 saved-world-transform restore (actor + pure-ECS)
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"     // FCk_Request_Transform_SetTransform (pure-ECS mover)

#include "CkCore/Algorithms/CkAlgorithms.h"                  // ck::algo::NoneOf

#include "CkLabel/CkLabel_Utils.h"                            // ConstructSpawned adopt/reconcile by label

#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
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
#include "UObject/SoftObjectPath.h" // FSoftClassPath::TryLoadClass for recipe/actor classes

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_subsystem
{
    constexpr auto UserIndex = 0;
    constexpr auto k_NoEntity = 0xFFFFFFFFu; // mirrors ck::snapshot's k_NoEntity sentinel

    // ECk_Snapshot_V3_Provenance carries no fmt formatter, so it cannot be printed via {} directly.
    auto
        DoProvenance_ToString(
            ECk_Snapshot_V3_Provenance InProvenance)
        -> const TCHAR*
    {
        switch (InProvenance)
        {
            case ECk_Snapshot_V3_Provenance::EngineOwned:      return TEXT("EngineOwned");
            case ECk_Snapshot_V3_Provenance::ConstructSpawned: return TEXT("ConstructSpawned");
            case ECk_Snapshot_V3_Provenance::RuntimeSpawned:   return TEXT("RuntimeSpawned");
            case ECk_Snapshot_V3_Provenance::DefinitionBuilt:  return TEXT("DefinitionBuilt");
        }
        return TEXT("Unknown");
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
                { return Entity; }
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

    // Defensive: never leave a world gated if we tear down mid-load.
    if (auto* EcsWorld = DoGet_LoadWorldEcs();
        ck::IsValid(EcsWorld))
    { EcsWorld->Set_IsLoadGateActive(false); }

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

    const auto HasAuthorityWorld = ck::IsValid(World) && ck_snapshot_subsystem::DoGet_HasWorldAuthority(World);
    CK_ENSURE_IF_NOT(HasAuthorityWorld,
        TEXT("Request_Save refused: World [{}] is invalid or this is a client (no authority)"), World)
    {}
    if (NOT HasAuthorityWorld)
    {
        InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_IO);
        return;
    }

    const auto CanStartSnapshot = NOT _SnapshotInProgress && NOT _LoadInProgress;
    CK_ENSURE_IF_NOT(CanStartSnapshot,
        TEXT("Request_Save refused: a snapshot operation is already in progress"))
    {}
    if (NOT CanStartSnapshot)
    {
        InDelegate.ExecuteIfBound(ECk_SnapshotResult::Failed_IO);
        return;
    }

    _SnapshotInProgress = true;
    ON_SCOPE_EXIT { _SnapshotInProgress = false; };

    auto Source = DoGet_SnapshotSource();
    ck::UUtils_Signal_Snapshot_OnPreSave::Broadcast(Source, ck::MakePayload(Source));

    if (auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        ck::IsValid(EcsWorld))
    {
        _LastPumpCount = EcsWorld->Request_PumpToQuiescence();
        ck::snapshot::Verbose(TEXT("Request_Save: pumped world to quiescence in [{}] pump passes"), _LastPumpCount);
    }

    auto ByteWriterV3 = FBufferArchive{};
    auto HeaderV3 = FCk_Snapshot_HeaderV3{};
    const auto CaptureResultV3 = ck::snapshot::Run_CaptureV3(*World, ByteWriterV3, HeaderV3);

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
    {}
    if (NOT HasSaveGame)
    {
        DoFinish(ECk_SnapshotResult::Failed_IO);
        return;
    }

    SaveGame->_HeaderV3 = HeaderV3;
    SaveGame->_SnapshotBytesV3 = MoveTemp(static_cast<TArray<uint8>&>(ByteWriterV3));

    const auto Saved = UGameplayStatics::SaveGameToSlot(SaveGame, InSlotName.ToString(), ck_snapshot_subsystem::UserIndex);
    if (NOT Saved)
    {
        ck::snapshot::Error(TEXT("Request_Save: SaveGameToSlot failed for slot [{}]"), InSlotName);
        DoFinish(ECk_SnapshotResult::Failed_IO);
        return;
    }

    ck::snapshot::Display(TEXT("Request_Save: saved [{}] v3 bytes to slot [{}]"),
        SaveGame->_SnapshotBytesV3.Num(), InSlotName);
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

    const auto HasAuthorityWorld = ck::IsValid(World) && ck_snapshot_subsystem::DoGet_HasWorldAuthority(World);
    CK_ENSURE_IF_NOT(HasAuthorityWorld,
        TEXT("Request_Load refused: World [{}] is invalid or this is a client (no authority)"), World)
    {}
    if (NOT HasAuthorityWorld)
    {
        InDelegate.ExecuteIfBound(MakeFailureReport(ECk_SnapshotResult::Failed_IO));
        return;
    }

    const auto CanStartSnapshot = NOT _SnapshotInProgress && NOT _LoadInProgress;
    CK_ENSURE_IF_NOT(CanStartSnapshot,
        TEXT("Request_Load refused: a snapshot operation is already in progress"))
    {}
    if (NOT CanStartSnapshot)
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
    _LoadPhase           = ELoadPhase::TearingDown;
    _PendingLoadDelegate = InDelegate;
    _LoadFrameCount      = 0;
    _PreTravelWorld      = nullptr;
    _TravelMapName.Reset();

    _SavedIdMap.Reset();
    _SpawnedRuntimeIds.Reset();
    _SkippedIds.Reset();
    _PersistedIds.Reset();
    for (const auto& Entry : _V3Tables.Get_Entities())
    { _PersistedIds.Add(Entry.Get_SavedId()); }
    _PendingBridgeActors.Reset();
    _V3LoadReport = FCk_Snapshot_LoadReport{};
    _V3LoadReport.Set_Result(ECk_SnapshotResult::Success);
    _HydrationEnqueued = false;
    _SettleFramesRemaining = 0;
    _SettleStarted = false;
    _RebuildLastMappedCount = 0;
    _RebuildStallTicks = 0;

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
                    if (NOT Entry.Get_ActorClassPath().IsEmpty()) { ++Bridged; }
                    break;
            }
        }
        ck::snapshot::Display(TEXT("DIAG: v3 table provenance — EngineOwned [{}], ConstructSpawned [{}], RuntimeSpawned [{}] (of which bridged [{}])"),
            EngineOwned, ConstructSpawned, RuntimeSpawned, Bridged);
    }

    auto Source = DoGet_SnapshotSource();
    ck::UUtils_Signal_Snapshot_OnPreLoad::Broadcast(Source, ck::MakePayload(Source));

    DoInitiate_Teardown();

    _LoadTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_Snapshot_Subsystem_UE::DoTick_Load));

    ck::snapshot::Display(TEXT("Request_Load: v3 load started for slot [{}] ([{}] entities, [{}] payloads; [{}] roots tearing down)"),
        InSlotName, _V3Tables.Get_Entities().Num(), _V3Tables.Get_Payloads().Num(), _PendingTeardownRoots.Num());
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
    _TravelMapName  = World->RemovePIEPrefix(World->GetOutermost()->GetName());

    constexpr auto AbsoluteTravel = true;

    // Seamless ServerTravel is connection-preserving but heavier — with no remote clients, OpenLevel is correct.
    auto* NetDriver = World->GetNetDriver();
    const auto HasConnectedClients =
        ck::IsValid(NetDriver, ck::IsValid_Policy_NullptrOnly{}) && NetDriver->ClientConnections.Num() > 0;

    if (World->GetNetMode() == ENetMode::NM_Standalone || NOT HasConnectedClients)
    {
        ck::snapshot::Display(TEXT("DIAG: Request_Load travel — OpenLevel (no connected clients) to map [{}] (pre-travel world [{}])"),
            _TravelMapName, World->GetName());
        UGameplayStatics::OpenLevel(World, FName{*_TravelMapName}, AbsoluteTravel);
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

    if (World == _PreTravelWorld.Get())
    { return false; }

    // Skip the seamless-travel transition map (different package name); the destination equals _TravelMapName.
    if (World->RemovePIEPrefix(World->GetOutermost()->GetName()) != _TravelMapName)
    { return false; }

    return World->HasBegunPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoRehydrate_SaveKeyResolver()
    -> void
{
    _SaveKeyResolverMap.Reset(); // pre-load entries point at pre-travel handles — dead after the world swap

    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EcsWorld))
    { return; }

    auto& CkRegistry = EcsWorld->Get_Registry();
    auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
    if (RawRegistry == nullptr)
    { return; }

    // The LIVE world-side SaveKey fragments (level actors the normal world build re-created), never restored ones.
    auto PublishedCount = 0;
    for (const auto Entity : RawRegistry->view<FFragment_SaveKey>())
    {
        const auto Handle = ck::MakeHandle(FCk_Entity{Entity}, CkRegistry);
        if (ck::Is_NOT_Valid(Handle))
        { continue; }

        const auto& Frag = RawRegistry->get<FFragment_SaveKey>(Entity);
        Publish_SaveKey(Frag.Get_Key(), Handle);
        ++PublishedCount;
    }

    ck::snapshot::Display(TEXT("DIAG: rehydrated SaveKey resolver with [{}] live entries"), PublishedCount);
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

    auto AnyUnresolved = false;

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
                const auto* Owner = _SavedIdMap.Find(Entry.Get_LifetimeOwnerSavedId());
                if (Owner != nullptr && ck::IsValid(*Owner) && Owner->Has<ck::FFragment_LifetimeDependents>())
                {
                    const auto& Label = Entry.Get_Label();
                    for (auto& Child : Owner->Get<ck::FFragment_LifetimeDependents>().Get_Entities())
                    {
                        if (ck::Is_NOT_Valid(Child) || NOT Child.Has<ck::FTag_ConstructSpawned>())
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
                            _SkippedIds.Add(SavedId);
                        }
                        else
                        {
                            auto SpawnInfo = FActorSpawnParameters{};
                            SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                            auto* Actor = World->SpawnActor<AActor>(ActorClass, Entry.Get_ActorSpawnTransform(), SpawnInfo);
                            if (Actor == nullptr)
                            {
                                ck::snapshot::Error(TEXT("v3 load: SpawnActor failed for bridged entity [{}] class [{}]"),
                                    SavedId, Entry.Get_ActorClassPath());
                                _SkippedIds.Add(SavedId);
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
                            _SkippedIds.Add(SavedId);
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
                                    _SkippedIds.Add(SavedId);
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
                        const auto Pending = UCk_Utils_EntityScript_UE::Request_SpawnEntity(Owner, ScriptClass, Params, {});
                        // The pending handle wraps the immediately-created entity (Construct completes over the pumps);
                        // map it now so dependents can reference it.
                        Resolved = Pending.Get_EntityUnderConstruction();
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
                        _SkippedIds.Add(SavedId);
                        break;
                    }
                    if (NOT _PersistedIds.Contains(OwnerSavedId))
                    {
                        // Data loss (the item is dropped), NOT boot-infra — flag it loudly rather than skip silently.
                        // The owner must persist: a named inventory, or a persisted context owner.
                        ck::snapshot::Warning(
                            TEXT("v3 load: DefinitionBuilt entity [{}] owner saved-id [{}] was not persisted — item dropped."),
                            SavedId, OwnerSavedId);
                        _SkippedIds.Add(SavedId);
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
                        _SkippedIds.Add(SavedId);
                        break;
                    }

                    // Via the lifetime owner ⇒ mirror production by building under ITS context owner.
                    auto BuildOwner = bBuildViaContextOwner
                        ? *MappedOwner
                        : UCk_Utils_ContextOwner_UE::Get_ContextOwner(*MappedOwner);
                    if (ck::Is_NOT_Valid(BuildOwner))
                    { BuildOwner = *MappedOwner; }

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
                        _SkippedIds.Add(SavedId);
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
                Resolved.AddOrReplace<FFragment_SaveKey>(Entry.Get_SaveKey());
                Publish_SaveKey(Entry.Get_SaveKey(), Resolved);
            }

            _SavedIdMap.Add(SavedId, Resolved);
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

    // Game-side rebind processors key off FTag_Snapshot_JustRestored to re-resolve handles their persisted fragments
    // carry; stamp it before the gate opens so the full post-gate pump sees it.
    for (const auto& Pair : _SavedIdMap)
    {
        if (auto Restored = Pair.Value;
            ck::IsValid(Restored))
        { Restored.AddOrGet<ck::FTag_Snapshot_JustRestored>(); }
    }

    auto EnqueuedCount = 0;
    auto PayloadsDropped = 0;
    for (const auto& Payload : _V3Tables.Get_Payloads())
    {
        auto* Owner = _SavedIdMap.Find(Payload.Get_OwnerSavedId());
        if (Owner == nullptr || ck::Is_NOT_Valid(*Owner))
        { continue; } // owner unmapped (orphan/skipped) — its payloads drop

        // A failed deserialize is lost state, not a no-op — counted in the load report even where the ensure is out.
        auto Data = DoDeserialize_V3Blob(Payload.Get_PayloadBytes());
        const auto HasHydrationData = Data.IsValid();
        CK_ENSURE_IF_NOT(HasHydrationData,
            TEXT("v3 load: hydration payload for type [{}] (owner saved-id [{}]) failed to deserialize — dropped "
                 "(empty bytes, or the type is absent since the save)"),
            Payload.Get_TypePath(), Payload.Get_OwnerSavedId())
        {}
        if (NOT HasHydrationData)
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

    // An orphan is a saved entity that never mapped AND was not deliberately skipped — skips are intentional (the
    // fresh world's boot owns them). Each orphan's payloads drop, and gets one Warning + one report record.
    auto OrphanIds = TSet<uint32>{};
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        const auto SavedId = Entry.Get_SavedId();
        if (NOT _SavedIdMap.Contains(SavedId) && NOT _SkippedIds.Contains(SavedId))
        { OrphanIds.Add(SavedId); }
    }

    auto Orphans = TArray<FCk_Snapshot_OrphanRecord>{};
    Orphans.Reserve(OrphanIds.Num());
    for (const auto& Entry : _V3Tables.Get_Entities())
    {
        const auto SavedId = Entry.Get_SavedId();
        if (NOT OrphanIds.Contains(SavedId))
        { continue; }

        const auto OwnerSavedId  = Entry.Get_LifetimeOwnerSavedId();
        const auto bOwnerOrphaned = OwnerSavedId != ck_snapshot_subsystem::k_NoEntity && OrphanIds.Contains(OwnerSavedId);

        auto Identity = FString{};
        auto Reason   = FString{};
        switch (Entry.Get_Provenance())
        {
            case ECk_Snapshot_V3_Provenance::EngineOwned:
            {
                const auto bHasSaveKey = Entry.Get_SaveKey().IsValid();
                Identity = bHasSaveKey ? Entry.Get_SaveKey().ToString() : Entry.Get_PlayerId();
                Reason   = bHasSaveKey ? TEXT("savekey-miss") : TEXT("player-miss");
                break;
            }
            case ECk_Snapshot_V3_Provenance::ConstructSpawned:
            {
                Identity = Entry.Get_Label();
                if (bOwnerOrphaned)                          { Reason = TEXT("owner-orphaned"); }          // cascade
                else if (_SavedIdMap.Contains(OwnerSavedId)) { Reason = TEXT("owner-mapped-label-miss"); } // content/label drift
                else                                         { Reason = TEXT("unresolved-other"); }
                break;
            }
            case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
            {
                const auto bBridged = NOT Entry.Get_ActorClassPath().IsEmpty();
                Identity = bBridged ? Entry.Get_ActorClassPath() : Entry.Get_ScriptClassPath();
                if (bBridged)            { Reason = TEXT("bridge-never-linked"); } // actor spawned, bridge never linked
                else if (bOwnerOrphaned) { Reason = TEXT("owner-orphaned"); }
                else                     { Reason = TEXT("unresolved-other"); }
                break;
            }
            case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
            {
                Identity = NOT Entry.Get_BuildRecipe().IsEmpty()
                    ? Entry.Get_BuildRecipe()[0].Get_ScriptClassPath()
                    : Entry.Get_ScriptClassPath();
                if (bOwnerOrphaned) { Reason = TEXT("owner-orphaned"); }
                else                { Reason = TEXT("unresolved-other"); }
                break;
            }
        }

        ck::snapshot::Warning(TEXT("v3 load ORPHAN: saved-id [{}] provenance [{}] identity [{}] owner [{}] reason [{}]"),
            SavedId, ck_snapshot_subsystem::DoProvenance_ToString(Entry.Get_Provenance()), Identity, OwnerSavedId, Reason);

        auto Record = FCk_Snapshot_OrphanRecord{};
        Record.Set_SavedId(SavedId);
        Record.Set_Provenance(Entry.Get_Provenance());
        Record.Set_Identity(Identity);
        Record.Set_OwnerSavedId(OwnerSavedId);
        Record.Set_Reason(Reason);
        Orphans.Add(MoveTemp(Record));
    }

    _V3LoadReport.Set_EntitiesRestored(_SavedIdMap.Num());
    _V3LoadReport.Set_EntitiesOrphaned(OrphanIds.Num());
    _V3LoadReport.Set_Orphans(MoveTemp(Orphans));
    _V3LoadReport.Set_PayloadsDropped(PayloadsDropped);
    ck::snapshot::Display(TEXT("DIAG: v3 hydrate — enqueued [{}] payloads across [{}] mapped entities ([{}] skipped boot-infra, [{}] orphaned, [{}] payloads dropped)"),
        EnqueuedCount, _SavedIdMap.Num(), _SkippedIds.Num(), _V3LoadReport.Get_EntitiesOrphaned(), PayloadsDropped);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_Subsystem_UE::
    DoIs_HydrationComplete() const
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
            if (auto* EcsWorld = DoGet_LoadWorldEcs();
                ck::IsValid(EcsWorld))
            { EcsWorld->Set_IsLoadGateActive(true); }

            DoRehydrate_SaveKeyResolver();

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

            if (NOT Complete)
            {
                auto UnEngine = 0, UnConstruct = 0, UnRuntime = 0, UnDefinitionBuilt = 0;
                for (const auto& Entry : _V3Tables.Get_Entities())
                {
                    if (_SavedIdMap.Contains(Entry.Get_SavedId()))
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
                ck::snapshot::Error(TEXT("Request_Load: rebuild {} — [{}]/[{}] mapped; unresolved by provenance: "
                    "EngineOwned [{}], ConstructSpawned [{}], RuntimeSpawned [{}], DefinitionBuilt [{}]. Proceeding (partial load)."),
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

            if (auto* EcsWorld = DoGet_LoadWorldEcs();
                ck::IsValid(EcsWorld))
            {
                EcsWorld->Set_IsLoadGateActive(false);
                EcsWorld->Request_PumpToQuiescence(ck::ECk_SchedulerTickScope::Full);
            }

            _SettleFramesRemaining = kLoad_SettleFrames;
            _SettleStarted = true;
            _LoadFrameCount = 0;
            _LoadPhase = ELoadPhase::Settling;
            return true;
        }

        case ELoadPhase::Settling:
        {
            // Finish only once hydration has fully drained AND the parked reconcile-destroys have had their minimum
            // settle frames. The frame cap is a LOUD abort backstop — reaching it means some payloads never applied.
            if (_SettleFramesRemaining > 0)
            { --_SettleFramesRemaining; }

            const auto HydrationPending = NOT DoIs_HydrationComplete();
            if ((HydrationPending || _SettleFramesRemaining > 0) && _LoadFrameCount < kLoad_HydrateFrameCap)
            { return true; }

            if (HydrationPending)
            {
                CK_TRIGGER_ENSURE(TEXT("Request_Load: settle hit the [{}]-frame cap with hydration still pending — "
                    "finishing anyway (some payloads did not apply)"), kLoad_HydrateFrameCap);
            }

            ck::snapshot::Display(TEXT("DIAG: v3 load settled — finishing (restored [{}], orphaned [{}])"),
                _V3LoadReport.Get_EntitiesRestored(), _V3LoadReport.Get_EntitiesOrphaned());
            DoFinish_Load(_V3LoadReport);
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
    // Defensive: a teardown/travel abort must never leave a world gated.
    if (auto* EcsWorld = DoGet_LoadWorldEcs();
        ck::IsValid(EcsWorld))
    { EcsWorld->Set_IsLoadGateActive(false); }

    _LoadTickerHandle.Reset(); // DoTick_Load returns false to unregister; just drop our copy of the handle
    _LoadPhase = ELoadPhase::Idle;
    _LoadInProgress = false;
    _PendingTeardownRoots.Reset();
    _V3Tables = FCk_Snapshot_V3_Tables{};
    _SavedIdMap.Reset();
    _SpawnedRuntimeIds.Reset();
    _SkippedIds.Reset();
    _PersistedIds.Reset();
    _PendingBridgeActors.Reset();

    const auto Delegate = _PendingLoadDelegate;
    _PendingLoadDelegate.Unbind();

    const auto Source = DoGet_SnapshotSource(); // re-resolve: the fresh world's transient
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
    if (ck::Is_NOT_Valid(SaveGame) || SaveGame->_SnapshotBytesV3.Num() == 0)
    { return {}; } // invalid slot, or a pre-v3 slot with no v3 header of record

    // The SaveGame stores only the v3 header; this frozen BP return type is synthesized from the six overlapping
    // fields. The legacy-only stream fields (manifest, transient id, tag offset) have no v3 source and stay default.
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
