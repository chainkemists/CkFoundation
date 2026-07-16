#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/CkJolt_ContactEvent.h"

#include <Async/Future.h>

#include "CkJolt_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class CkJoltDebugger;
class BPLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class CkObjectLayerPairFilterImpl;
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
    TPimplPtr<BPLayerInterfaceImpl> _BroadPhaseLayerInterface;
    TPimplPtr<ObjectVsBroadPhaseLayerFilterImpl> _ObjectVsBroadPhaseLayerFilter;
    TPimplPtr<CkObjectLayerPairFilterImpl> _ObjectVsObjectFilter;
    TSharedPtr<JPH::PhysicsSystem> _PhysicsSystem;

    TPimplPtr<CkBodyActivationListener> _BodyActivationListener;
    TPimplPtr<CkContactListener> _ContactListener;

    int32 _CollisionSteps = 1;

    TFuture<void> _PhysicsAsyncFuture;
    bool _ParallelPhysicsEnabled = false;
    int32 _PhysicsThreadCount = 0;
    bool _AsyncPhysicsUpdate = false;

    // Consumers of drained contact events (e.g. CkSpatialQuery's probe-overlap translation).
    // Broadcast on the game thread in Tick, before this frame's physics update.
    FCk_Jolt_OnContactEventsDrained _OnContactEventsDrained;

    // Debug-draw opt-in installed by a consumer (the Jolt world itself has no user-facing debug
    // toggle — CkSpatialQuery's user settings own that today). No gate installed = no debug draw.
    TFunction<bool()> _DebugDrawGate;

#if JPH_DEBUG_RENDERER
    TPimplPtr<CkJoltDebugger> _Debugger;
#endif

public:
    auto
    Get_PhysicsSystem() const -> TWeakPtr<JPH::PhysicsSystem>;

    auto
    Get_OnContactEventsDrained() -> FCk_Jolt_OnContactEventsDrained&;

    auto
    Set_DebugDrawGate(
        TFunction<bool()> InGate) -> void;

    CK_PROPERTY_GET(_ParallelPhysicsEnabled);
    CK_PROPERTY_GET(_PhysicsThreadCount);
    CK_PROPERTY_GET(_AsyncPhysicsUpdate);
};

// --------------------------------------------------------------------------------------------------------------------
