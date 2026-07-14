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
    // Stored by VALUE (not pointer). FCk_Registry is now a trivially-copyable
    // (slot+gen + transient entity) view; copying it is cheap and severs the
    // actor's lifetime coupling to the subsystem. If the slot is freed (e.g.
    // subsystem deinitialised before the actor is GC'd), Resolve returns null
    // and ck::IsValid(_Registry) returns false — the tick guard fails closed.
    FCk_Registry _Registry;

    TStatId _TickStatId;
    FString _TickStatName;
    ETickingGroup _UnrealTickingGroup = TG_PrePhysics;

    FGameplayTag _EcsWorldTickingGroup;
    ECk_Ecs_WorldStatCollection_Policy _StatCollectionPolicy = ECk_Ecs_WorldStatCollection_Policy::DoNotCollect;
    FName _EcsWorldDisplayName;

    // Lazily resolved (first Tick) + cached back-reference to this world's ECS subsystem, so the per-frame tick can
    // read the load gate and pick the scheduler scope without a per-tick GetSubsystem lookup.
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
    // Fired just before DoBuildGraphAndSpawnActors calls FProcessorGraphBuilder::Build. Subscribers may
    // register additional FProcessorDescriptors with ck::FProcessorRegistry before the graph is assembled.
    // This is the hook CkDynamic uses to inject AngelScript/Blueprint-defined script processors into the
    // pipeline without creating a CkEcs → CkDynamic dependency cycle.
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

    // Request a full teardown and rebuild of the processor graph at the end of the current frame.
    // Safe to call mid-tick — actual rebuild is deferred until FCoreDelegates::OnEndFrame.
    // Idempotent: multiple calls within the same frame collapse into a single rebuild.
    auto
    Request_RebuildProcessorGraph() -> void;

    // Pumps every ticking-group scheduler with DeltaTime=0 so all pending deferred requests
    // drain to quiescence WITHOUT advancing game time. Used by CkSnapshot before a capture so
    // the snapshot reflects a settled (consistent) world. Each scheduler's Tick already loops
    // its internal DoPump to quiescence (cap _MaxPumpIterations) and logs still-dirty processors.
    // Returns the total pump-pass count across all schedulers (0 == already quiescent;
    // a high count == lots of cascading work drained). Safe: skips any scheduler whose tick is
    // already in progress (re-entrancy guard).
    //
    // InScope: Full (default — pre-save/normal drain, every processor) or LoadKernel (the load orchestrator's
    // rebuild drain — only RunsDuringLoad processors, spec §4.3).
    auto
    Request_PumpToQuiescence(
        ck::ECk_SchedulerTickScope InScope = ck::ECk_SchedulerTickScope::Full) -> int32;

    // Worst-case (maximum) pump-pass count across all per-ticking-group schedulers for the most recent
    // frame. 0 when no schedulers exist (e.g. menu world). Surfaced live by CkWatermark to show pump
    // pressure; mirrors the per-scheduler condition that drives the throttled pump-limit log warning.
    auto Get_WorstFramePumpCount() const -> int32;

    // The per-frame pump-iteration budget (max across schedulers). 0 when no schedulers exist.
    auto Get_MaxPumpIterations() const -> int32;

    // Re-point this world's ECS bookkeeping at a transient entity that a snapshot restore rebuilt into the
    // registry. Overwrites FCtx_TransientEntity (via insert_or_assign — entt's emplace would NOT overwrite the
    // stale post-clear ctx), refreshes the cached _TransientEntity handle, and re-attaches the world-fragment.
    // Used by Run_Restore(UWorld&) after the continuous_loader rebuilds the entity set.
    auto
    Request_AdoptRestoredTransient(
        FCk_Entity InRestoredTransient) -> void;

    // Load gate (spec §4.3): while active, every EcsWorld actor ticks its scheduler with LoadKernel scope, so only
    // RunsDuringLoad kernel processors run — feature processors are frozen against the half-rebuilt world. Set by the
    // CkSnapshot load orchestrator (Phase 3B) across the rebuild; cleared once reconciliation completes.
    auto Get_IsLoadGateActive() const -> bool;
    auto Set_IsLoadGateActive(bool InActive) -> void;

private:
    auto DoBuildGraphAndSpawnActors(
        UWorld& InWorld) -> void;

    // Tears down all world actors + schedulers then calls DoBuildGraphAndSpawnActors.
    auto DoTeardownAndRebuild(
        UWorld& InWorld) -> void;

    // Bound to FCoreDelegates::OnEndFrame when a rebuild is pending.
    auto OnEndFrame_DoRebuild() -> void;

private:
    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

private:
    TMap<TEnumAsByte<ETickingGroup>, TStrongObjectPtr<ACk_EcsWorld_Actor_UE>> _WorldActors;

    bool _PendingRebuildGraph = false;
    FDelegateHandle _OnEndFrameHandle;

    // Phase 2: set by the CkSnapshot load orchestrator; read per-frame by each EcsWorld actor to pick the scheduler
    // tick scope (LoadKernel while active). See Get_/Set_IsLoadGateActive.
    bool _IsLoadGateActive = false;

private:
    // Owns the underlying entt registry. Slot is registered with
    // ck::registry_table on Initialize; freed on Deinitialize. _Registry below
    // is a non-owning view (slot+gen) bound to this owned registry.
    TUniquePtr<ck::registry_table::EnttRegistryType> _OwnedRegistry;
    FCk_Registry _Registry;

public:
    CK_PROPERTY_GET(_TransientEntity);
    CK_PROPERTY_GET(_WorldActors);
    CK_PROPERTY_GET(_Registry);
    CK_PROPERTY_GET_NON_CONST(_Registry);
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
