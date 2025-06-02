#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkSpatialQuery_Subsystem.generated.h"

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

UCLASS(DisplayName = "CkSubsystem_SpatialQuery")
class CKSPATIALQUERY_API UCk_SpatialQuery_Subsystem : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SpatialQuery_Subsystem);

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

#if JPH_DEBUG_RENDERER
    TPimplPtr<CkJoltDebugger> _Debugger;
#endif

public:
    auto
    Get_PhysicsSystem() const -> TWeakPtr<JPH::PhysicsSystem>;
};

// --------------------------------------------------------------------------------------------------------------------
