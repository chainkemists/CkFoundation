#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Scheduler/CkProcessorScheduler.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>
#include <GameplayTags.h>
#include <Templates/Function.h>

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

// Which phase of a snapshot load owns this world right now. ONE value spans the whole load, in every world the
// load passes through, and it is the only input to what a world-actor tick is allowed to do (Get_TickPlanForHold).
// The phases are ordered by the load's own progress, not by severity.
UENUM()
enum class ECk_EcsWorld_LoadHold : uint8
{
    // Normal play. Full scope, real time.
    None,
    // The world being demolished. Full scope — the destruction pipeline, every feature EndPlay and the entity
    // lifecycle groups are outside the load kernel, so a kernel-scoped teardown wedges — but at ZERO time.
    Teardown,
    // The world being rebuilt. LoadKernel scope: only the kernel processors run, so no feature processor ever
    // observes the half-rebuilt world.
    Rebuilding,
    // Rebuilding, escalated: the kernel quiesced with saved rows still unresolved, so the FULL scope runs to let
    // multi-stage constructions (and the identity they stamp on completion) finish — at ZERO time, because
    // time-paced world policy must not act on a census taken mid-rebuild.
    Escalated,
    // Payloads applying: the restored values, the deferred requests those applies issue, and the parked
    // reconcile-destroys. Full scope, ZERO time.
    Draining,
    // Drained, not yet coherent: physics bodies, probe overlaps and the deferred queues converge before the
    // player is handed the world back. Full scope, ZERO time.
    Converging,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EcsWorld_LoadHold);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // What ONE world-actor tick does under a given hold: which processors are eligible, and how much time they are
    // handed.
    struct FCk_TickPlan
    {
        CK_GENERATED_BODY(FCk_TickPlan);

        ECk_SchedulerTickScope _Scope = ECk_SchedulerTickScope::Full;
        FCk_Time _TickTime;
    };

    // A PURE function of the hold, so the whole policy is one table that can be asserted without a world. The
    // zeroed tick time is reinforcement rather than the mechanism — the loader also freezes the engine's global
    // time dilation, so the incoming DeltaSeconds is already ~0 — but it keeps the ECS deterministic in the frame
    // between raising the hold and the dilation write, and in any world where dilation is disabled outright.
    CKECS_API auto
    Get_TickPlanForHold(
        ECk_EcsWorld_LoadHold InHold,
        float InDeltaSeconds) -> FCk_TickPlan;
}

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

    // Tick groups the most recent Request_PumpToQuiescence could NOT pump because their tick was already in
    // progress. Recorded rather than merely logged because a caller reading the pump's count to decide whether
    // the world is quiescent would otherwise read a skipped group as a quiet one: a group that never ran
    // contributes 0 passes, which is indistinguishable from a group that ran and found nothing to do.
    auto Get_LastPumpSkippedGroupCount() const -> int32;

    // The per-frame pump-iteration budget (max across schedulers). 0 when no schedulers exist.
    auto Get_MaxPumpIterations() const -> int32;

    // Which phase of a snapshot load owns this world. The snapshot load orchestrator is the only writer; every
    // world-actor tick reads it exactly once and does what Get_TickPlanForHold says.
    auto Get_LoadHold() const -> ECk_EcsWorld_LoadHold;
    auto Set_LoadHold(ECk_EcsWorld_LoadHold InHold) -> void;

    // True for every phase of a load, i.e. Get_LoadHold() != None.
    auto Get_IsLoadGateActive() const -> bool;

    // The escalated rebuild pass: the kernel quiesced with saved rows still unresolved, so the FULL processor
    // scope runs AT ZERO TIME. A multi-stage construction (EntityScript `Continue` fulfilled by a game
    // processor) cannot finish under the kernel, and the identity it late-stamps (a GameplayLabel adopt key, a
    // SaveKey) is the very thing the rebuild is waiting on.
    auto Get_IsLoadGateEscalated() const -> bool;

    // Compatibility setters over the phase above, kept so callers that only ever expressed "the gate is up" and
    // "the world is escalating" keep saying exactly that. true -> Rebuilding (the phase whose spawn-suppression
    // and kernel-scope semantics those callers mean by "the gate"), false -> None; escalating -> Escalated, and
    // un-escalating returns to Rebuilding. Lowering the gate clears escalation, as it always did.
    auto Set_IsLoadGateActive(bool InActive) -> void;
    auto Set_IsLoadGateEscalated(bool InEscalated) -> void;

    // Answered at OnWorldBeginPlay, BEFORE the world's processor graph and scheduler actors exist: "does a load
    // own this world from its very first frame?". A world that comes up mid-load has to be held before anything
    // in it ticks, and the loader cannot do that itself — its own next opportunity is a full world tick later,
    // which is the frame every level actor's BeginPlay and every entity-script construction runs in.
    //
    // SIDE-EFFECTING by design: the provider applies whatever else its owner needs applied to the fresh world
    // (the global time dilation a load holds does not survive travel and has to be re-applied here) and returns
    // whether the world is held. Registered from the owning module's startup and cleared on its shutdown, so
    // CkEcs keeps no dependency on the module that answers.
    using FLoadHoldSeedProvider = TFunction<bool(UWorld&)>;
    static auto Set_LoadHoldSeedProvider(FLoadHoldSeedProvider InProvider) -> void;
    static auto Clear_LoadHoldSeedProvider() -> void;

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
    static auto DoGet_LoadHoldSeedProvider() -> FLoadHoldSeedProvider&;

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

    ECk_EcsWorld_LoadHold _LoadHold = ECk_EcsWorld_LoadHold::None;
    int32 _LastPumpSkippedGroupCount = 0;
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
    // Test/diagnostic hooks over the scheduler's per-frame debug history (300-frame ring); -1 when
    // the frame or processor is not in the history. Reading through these arms the demand-driven
    // timing collection, but only from the NEXT frame — a caller asking about a PAST frame gets
    // 0.0 ms elapsed unless collection was already running. Counts (entities, pumps, dirty,
    // empty-view skips) are always recorded.
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
