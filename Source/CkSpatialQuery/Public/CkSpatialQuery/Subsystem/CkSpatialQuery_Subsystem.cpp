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
    _Config = edyn::init_config{};
    _Config.execution_mode = edyn::execution_mode::asynchronous;

    _PhysicsWorld.Get_Registry().Request_PerformOperationOnInternalRegistry([&](entt::registry& InRegistry)
    {
        edyn::attach(InRegistry, _Config);
    });
}

auto
    UCk_SpatialQuery_Subsystem_UE::
    Deinitialize()
    -> void
{
    _PhysicsWorld.Get_Registry().Request_PerformOperationOnInternalRegistry([&](entt::registry& InRegistry)
    {
        edyn::detach(InRegistry);
    });
}

// --------------------------------------------------------------------------------------------------------------------
