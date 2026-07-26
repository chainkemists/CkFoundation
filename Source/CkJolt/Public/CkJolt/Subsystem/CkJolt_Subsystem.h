#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"
#include "CkJolt/World/CkJoltWorld.h"

#include "CkJolt_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class CkJoltDebugger;
class CkContactListener;
class CkBodyActivationListener;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class TempAllocatorImpl;
    class JobSystem;
    class ObjectLayerPairFilterImpl;
    class PhysicsSystem;
    class BodyInterface;

    class Vec3;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Jolt")
class CKJOLT_API UCk_Jolt_Subsystem : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Jolt_Subsystem);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Tick(
        float InDeltaTime) -> void override;

    auto
    Deinitialize() -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

private:
    TPimplPtr<JPH::TempAllocatorImpl> _TempAllocator;
    JPH::JobSystem* _JobSystem = nullptr;
    TUniquePtr<ck::jolt::FCk_Jolt_CollisionLayerTable> _LayerTable;
    TUniquePtr<ck::jolt::FCk_Jolt_BroadPhaseLayerInterface_Table> _BroadPhaseLayerInterface;
    TUniquePtr<ck::jolt::FCk_Jolt_ObjectVsBroadPhaseLayerFilter_Table> _ObjectVsBroadPhaseLayerFilter;
    TUniquePtr<ck::jolt::FCk_Jolt_ObjectLayerPairFilter_Table> _ObjectVsObjectFilter;
    TSharedPtr<JPH::PhysicsSystem> _PhysicsSystem;

    TPimplPtr<CkBodyActivationListener> _BodyActivationListener;
    TPimplPtr<CkContactListener> _ContactListener;

    int32 _CollisionSteps = 1;

    bool _ParallelPhysicsEnabled = false;
    int32 _PhysicsThreadCount = 0;
    bool _AsyncPhysicsUpdate = false;

    // Published to the ECS registry as a TSharedPtr context; holds non-owning pointers into the members above.
    TSharedPtr<ck::FJoltWorld> _JoltWorld;

    // Consumer-installed debug-draw opt-in. No gate installed = no debug draw.
    TFunction<bool()> _DebugDrawGate;

#if JPH_DEBUG_RENDERER
    TPimplPtr<CkJoltDebugger> _Debugger;
#endif

public:
    auto
    Get_PhysicsSystem() const -> TWeakPtr<JPH::PhysicsSystem>;

    /// Register a consumer of drained contact events (e.g. CkSpatialQuery's probe-overlap
    /// translation). Routers fire on the game thread, in registration order. Names must be unique.
    auto
    RegisterContactRouter(
        FName InName,
        ck::FCk_Jolt_ContactEventRouter InRouter) -> void;

    auto
    UnregisterContactRouter(
        FName InName) -> void;

    /// Signature-driven layer table (one JPH::ObjectLayer per unique collision signature).
    /// Registration is game-thread only; Jolt filters read it lock-free from workers.
    auto
    Get_LayerTable() -> ck::jolt::FCk_Jolt_CollisionLayerTable&;

    /// Runs PhysicsSystem::OptimizeBroadPhase on the game thread immediately before the next
    /// Update (never while an async update is in flight). Callers batch: request once after
    /// bulk add/remove, not per body.
    auto
    Request_OptimizeBroadPhaseBeforeNextUpdate() -> void;

    auto
    Set_DebugDrawGate(
        TFunction<bool()> InGate) -> void;

    CK_PROPERTY_GET(_ParallelPhysicsEnabled);
    CK_PROPERTY_GET(_PhysicsThreadCount);
    CK_PROPERTY_GET(_AsyncPhysicsUpdate);
};

// --------------------------------------------------------------------------------------------------------------------
