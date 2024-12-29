#include "CkSpatialQuery_Subsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#ifdef JPH_EXPORT
#error "JPH_EXPORT already defined"
#endif

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SpatialQuery_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    using namespace JPH;

    RegisterDefaultAllocator();
    Factory::sInstance = new Factory{};

    RegisterTypes();
}

auto
    UCk_SpatialQuery_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    Super::Tick(InDeltaTime);
}

auto
    UCk_SpatialQuery_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------
