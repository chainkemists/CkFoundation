#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Scheduler/CkProcessorScheduler.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>
#include <GameplayTags.h>

#include "CkEcsWorld_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_WorldStatCollection_Policy : uint8
{
    DoNotCollect,
    CollectOnLocalClientOnly,
    CollectOnServerOnly,
    CollectOnLocalClientAndServer
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ecs_WorldStatCollection_Policy);

// --------------------------------------------------------------------------------------------------------------------

class UCk_EcsWorld_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKECS_API ACk_EcsWorld_Actor_UE final : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_EcsWorld_Actor_UE);

public:
    friend class UCk_EcsWorld_Subsystem_UE;
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    ACk_EcsWorld_Actor_UE();

protected:
    auto
    Tick(
        float DeltaSeconds) -> void override;

public:
    auto
    Initialize(
        ck::FProcessorScheduler&& InScheduler,
        const FCk_Registry& InRegistry,
        ETickingGroup InTickGroup) -> void;

private:
    TOptional<ck::FProcessorScheduler> _Scheduler;
    // By VALUE deliberately: the copy severs the actor's lifetime coupling to the subsystem, and a
    // freed slot resolves to null so ck::IsValid fails and the tick guard fails closed.
    FCk_Registry _Registry;

    TStatId _TickStatId;
    FString _TickStatName;
    ETickingGroup _UnrealTickingGroup = TG_PrePhysics;

    FGameplayTag _EcsWorldTickingGroup;
    ECk_Ecs_WorldStatCollection_Policy _StatCollectionPolicy = ECk_Ecs_WorldStatCollection_Policy::DoNotCollect;
    FName _EcsWorldDisplayName;

    // Lazily resolved on first Tick, so reading the load gate costs no per-tick GetSubsystem lookup.
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _OwningSubsystem;

    auto DoGet_OwningSubsystem() -> const UCk_EcsWorld_Subsystem_UE*;

public:
    CK_PROPERTY_GET(_Scheduler);
    CK_PROPERTY_GET(_UnrealTickingGroup);
    CK_PROPERTY_GET(_TickStatName);
    CK_PROPERTY_GET(_EcsWorldTickingGroup);
    CK_PROPERTY_GET(_StatCollectionPolicy);
    CK_PROPERTY_GET(_EcsWorldDisplayName);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_EcsWorld")
class CKECS_API UCk_EcsWorld_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    // Fired just before FProcessorGraphBuilder::Build; subscribers may register additional descriptors.
    // Exists so CkDynamic can inject script processors without a CkEcs → CkDynamic dependency cycle.
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreBuildProcessorGraph, UWorld& /*InWorld*/);

    static auto Get_OnPreBuildProcessorGraph() -> FOnPreBuildProcessorGraph&;

public:
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;
    auto
    Deinitialize() -> void override;

    auto
    OnWorldBeginPlay(
        UWorld& InWorld) -> void override;

    // Safe to call mid-tick — the rebuild is deferred to FCoreDelegates::OnEndFrame, and repeat calls
    // within one frame collapse into a single rebuild.
    auto
    Request_RebuildProcessorGraph() -> void;

    // Pumps every ticking-group scheduler with DeltaTime=0 so pending deferred requests drain to
    // quiescence WITHOUT advancing game time — CkSnapshot uses it so a capture sees a settled world.
    // Returns the total pump-pass count across all schedulers (0 == already quiescent). Any scheduler
    // whose tick is already in progress is skipped.
    auto
    Request_PumpToQuiescence(
        ck::ECk_SchedulerTickScope InScope = ck::ECk_SchedulerTickScope::Full) -> int32;

    // Maximum pump-pass count across schedulers for the most recent frame; 0 when none exist (e.g. a
    // menu world). Mirrors the per-scheduler condition behind the throttled pump-limit log warning.
    auto Get_WorstFramePumpCount() const -> int32;

    // The per-frame pump-iteration budget (max across schedulers). 0 when no schedulers exist.
    auto Get_MaxPumpIterations() const -> int32;

    // Re-points this world's ECS bookkeeping at a transient entity a snapshot restore rebuilt into the
    // registry. Called by Run_Restore(UWorld&) once the loader has rebuilt the entity set.
    auto
    Request_AdoptRestoredTransient(
        FCk_Entity InRestoredTransient) -> void;

    // While active, every EcsWorld actor ticks with LoadKernel scope so feature processors stay frozen
    // against the half-rebuilt world. Held by the snapshot load orchestrator across the rebuild.
    auto Get_IsLoadGateActive() const -> bool;
    auto Set_IsLoadGateActive(bool InActive) -> void;

    // Escalated rebuild: the load gate stays owned by the orchestrator, but the world ticks the FULL
    // processor scope AT ZERO TIME. The loader escalates when the kernel quiesces with saved rows still
    // unresolved — a multi-stage construction (EntityScript `Continue` fulfilled by a game processor) can
    // only finish under the full graph, and the identity it late-stamps (a GameplayLabel adopt key, a
    // SaveKey) is the very thing the rebuild is waiting on. Zero time keeps time-paced world policy from
    // observing the half-rebuilt world. Cleared automatically when the gate deactivates.
    auto Get_IsLoadGateEscalated() const -> bool;
    auto Set_IsLoadGateEscalated(bool InEscalated) -> void;

    // While the load gate is active, the loader is the sole creator of world population: only spawns issued
    // inside its own window (recipe replays, definition rebuilds) or by an owner still inside its construction
    // window are admitted — everything else is world policy reacting to the half-rebuilt world and is
    // suppressed (see Request_SpawnEntity). Scope-managed via FCk_ScopedLoaderSpawnWindow; never set directly.
    auto Get_IsInLoaderSpawnWindow() const -> bool;
    auto Push_LoaderSpawnWindow() -> void;
    auto Pop_LoaderSpawnWindow() -> void;

    // Declared load admission for RENDEZVOUS spawns: world bootstrap re-creating identity-bearing content
    // the loader ADOPTS instead of respawning (level-placed spawner entities with SaveKeys, keyed world
    // singletons). Suppressing these starves the rebuild — their saved rows wait on a rendezvous that never
    // comes. Scope-managed via FCk_ScopedRendezvousSpawnWindow; never set directly.
    auto Get_IsInRendezvousSpawnWindow() const -> bool;
    auto Push_RendezvousSpawnWindow() -> void;
    auto Pop_RendezvousSpawnWindow() -> void;

private:
    auto DoBuildGraphAndSpawnActors(
        UWorld& InWorld) -> void;

    auto DoTeardownAndRebuild(
        UWorld& InWorld) -> void;

    auto OnEndFrame_DoRebuild() -> void;

private:
    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

private:
    TMap<TEnumAsByte<ETickingGroup>, TStrongObjectPtr<ACk_EcsWorld_Actor_UE>> _WorldActors;

    bool _PendingRebuildGraph = false;
    FDelegateHandle _OnEndFrameHandle;

    bool _IsLoadGateActive = false;
    bool _IsLoadGateEscalated = false;
    int32 _LoaderSpawnWindowDepth = 0;
    int32 _RendezvousSpawnWindowDepth = 0;

private:
    // _Registry below is a non-owning (slot+gen) view bound to this owned registry.
    TUniquePtr<ck::registry_table::EnttRegistryType> _OwnedRegistry;
    FCk_Registry _Registry;

public:
    CK_PROPERTY_GET(_TransientEntity);
    CK_PROPERTY_GET(_WorldActors);
    CK_PROPERTY_GET(_Registry);
    CK_PROPERTY_GET_NON_CONST(_Registry);
};

// --------------------------------------------------------------------------------------------------------------------

// RAII loader-spawn window: spawns issued inside this scope are the LOADER's own world reconstitution (recipe
// replays, definition rebuilds) and pass the load-gate spawn suppression. The snapshot load orchestrator owns
// the only legitimate call sites — game code must never open one.
struct CKECS_API FCk_ScopedLoaderSpawnWindow
{
    explicit FCk_ScopedLoaderSpawnWindow(UCk_EcsWorld_Subsystem_UE* InSubsystem)
        : _Subsystem(InSubsystem)
    {
        if (_Subsystem.IsValid())
        { _Subsystem->Push_LoaderSpawnWindow(); }
    }

    ~FCk_ScopedLoaderSpawnWindow()
    {
        if (_Subsystem.IsValid())
        { _Subsystem->Pop_LoaderSpawnWindow(); }
    }

    FCk_ScopedLoaderSpawnWindow(const FCk_ScopedLoaderSpawnWindow&) = delete;
    FCk_ScopedLoaderSpawnWindow& operator=(const FCk_ScopedLoaderSpawnWindow&) = delete;
    FCk_ScopedLoaderSpawnWindow(FCk_ScopedLoaderSpawnWindow&&) = delete;
    FCk_ScopedLoaderSpawnWindow& operator=(FCk_ScopedLoaderSpawnWindow&&) = delete;

private:
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _Subsystem;
};

// --------------------------------------------------------------------------------------------------------------------

// RAII declared-admission window for RENDEZVOUS spawns during a load: the caller asserts the spawned entity
// carries (or acquires during construction) a stable identity — a SaveKey or an (owner, label) adopt key —
// so the loader ADOPTS it rather than respawning it, and admitting it mid-load cannot double the world.
// Census/count-driven population spawns must NEVER open this window; they stay suppressed by design.
struct CKECS_API FCk_ScopedRendezvousSpawnWindow
{
    explicit FCk_ScopedRendezvousSpawnWindow(UCk_EcsWorld_Subsystem_UE* InSubsystem)
        : _Subsystem(InSubsystem)
    {
        if (_Subsystem.IsValid())
        { _Subsystem->Push_RendezvousSpawnWindow(); }
    }

    ~FCk_ScopedRendezvousSpawnWindow()
    {
        if (_Subsystem.IsValid())
        { _Subsystem->Pop_RendezvousSpawnWindow(); }
    }

    FCk_ScopedRendezvousSpawnWindow(const FCk_ScopedRendezvousSpawnWindow&) = delete;
    FCk_ScopedRendezvousSpawnWindow& operator=(const FCk_ScopedRendezvousSpawnWindow&) = delete;
    FCk_ScopedRendezvousSpawnWindow(FCk_ScopedRendezvousSpawnWindow&&) = delete;
    FCk_ScopedRendezvousSpawnWindow& operator=(FCk_ScopedRendezvousSpawnWindow&&) = delete;

private:
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _Subsystem;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EcsWorld_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_EcsWorld_Subsystem_UE;

public:
    static auto
    Get_TransientEntity(
        const UWorld* InWorld) -> FCk_Handle;

    UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle
    Get_TransientEntity_FromContextObject(
        const UObject* InWorldContextObject);

public:
    // Reading through these arms the scheduler's demand-driven timing collection, but only from the
    // NEXT frame — a caller asking about a PAST frame gets 0.0 ms elapsed unless collection was
    // already running. Counts (entities, pumps, dirty, empty-view skips) are always recorded.
    // Test/diagnostic hooks over the scheduler's per-frame debug history (ck.Scheduler.DebugTiming,
    // default on; 300-frame ring). -1 when the frame or processor is not in the history.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EcsWorld|Debug",
              DisplayName="[Ck][EcsWorld] Get Debug Processor Pump Count For Frame")
    static int32
    Get_Debug_ProcessorPumpCountForFrame(const UObject* InWorldContextObject, FName InProcessorName, int64 InFrameNumber);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EcsWorld|Debug",
              DisplayName="[Ck][EcsWorld] Get Debug Processor Main Pass Entity Count For Frame")
    static int32
    Get_Debug_ProcessorMainPassEntityCountForFrame(const UObject* InWorldContextObject, FName InProcessorName, int64 InFrameNumber);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|EcsWorld|Debug",
              DisplayName="[Ck][EcsWorld] Get Debug Frame Pump Iteration Count")
    static int32
    Get_Debug_FramePumpIterationCount(const UObject* InWorldContextObject, int64 InFrameNumber);

public:
    template <typename T_SubsystemClass>
    [[nodiscard]]
    static auto
    Get_WorldSubsystem(
        const FCk_Handle& InAnyHandle) -> T_SubsystemClass*
    ;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_SubsystemClass>
auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_WorldSubsystem(
        const FCk_Handle& InAnyHandle)
    -> T_SubsystemClass*
{
    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("Unable to a valid World [{}] from Handle [{}]"), World, InAnyHandle)
    { return nullptr; }

    return World->GetSubsystem<T_SubsystemClass>();
}

// --------------------------------------------------------------------------------------------------------------------
