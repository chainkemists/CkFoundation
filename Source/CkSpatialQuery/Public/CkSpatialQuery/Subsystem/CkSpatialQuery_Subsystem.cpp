#include "CkSpatialQuery_Subsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <edyn/edyn.hpp>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SpatialQuery_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    _Config = edyn::init_config{};
    _Config.execution_mode = edyn::execution_mode::sequential;
    _Config.num_worker_threads = 1;

    _EcsWorld.Get_Registry().Request_PerformOperationOnInternalRegistry([&](entt::registry& InRegistry)
    {
        edyn::attach(InRegistry, _Config);
    });

    edyn::rigidbody_def{};
}

auto
    UCk_SpatialQuery_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    Super::Tick(InDeltaTime);

    _EcsWorld.Get_Registry().Request_PerformOperationOnInternalRegistry([&](entt::registry& InRegistry)
    {
        edyn::update(InRegistry);
    });
}

auto
    UCk_SpatialQuery_Subsystem_UE::
    Deinitialize()
    -> void
{
    _EcsWorld.Get_Registry().Request_PerformOperationOnInternalRegistry([&](entt::registry& InRegistry)
    {
        edyn::detach(InRegistry);
    });

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------
