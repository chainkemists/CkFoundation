#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkSpatialQuery_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class BPLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class CkObjectLayerPairFilterImpl;
class CkContactListener;
class CkBodyActivationListener;

// ReSharper disable once CppInconsistentNaming
namespace JPH
{
    class TempAllocatorImpl;
    class JobSystemThreadPool;
    class ObjectLayerPairFilterImpl;
    class PhysicsSystem;
    class BodyInterface;

    class Vec3;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKSPATIALQUERY_API UCk_SpatialQuery_Subsystem_UE : public UCk_Game_TickableWorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SpatialQuery_Subsystem_UE);

public:
    auto
        Initialize(
            FSubsystemCollectionBase& InCollection)
            -> void override;

    auto
        Tick(
            float InDeltaTime)
            -> void override;

    auto
        Deinitialize()
            -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

private:
    TUniquePtr<JPH::TempAllocatorImpl> _TempAllocator;
    TUniquePtr<JPH::JobSystemThreadPool> _JobSystem;
    TUniquePtr<BPLayerInterfaceImpl> _BroadPhaseLayerInterface;
    TUniquePtr<ObjectVsBroadPhaseLayerFilterImpl> _ObjectVsBroadPhaseLayerFilter;
    TUniquePtr<CkObjectLayerPairFilterImpl> _ObjectVsObjectFilter;
    TSharedPtr<JPH::PhysicsSystem> _PhysicsSystem;

    TUniquePtr<CkBodyActivationListener> _BodyActivationListener;
    TUniquePtr<CkContactListener> _ContactListener;

public:
    auto
        Get_PhysicsSystem() const
            -> TWeakPtr<JPH::PhysicsSystem>;
};

// --------------------------------------------------------------------------------------------------------------------
