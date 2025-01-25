#include "CkProbe_ProcessorInjector.h"

#include "CkSpatialQuery/Probe/CkProbe_Processor.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Probe_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    auto SpatialQuerySubsystem = GetWorld()->GetSubsystem<UCk_SpatialQuery_Subsystem>();
    if (ck::Is_NOT_Valid(SpatialQuerySubsystem))
    { return; }

    InWorld.Add<ck::FProcessor_Probe_Setup>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_Probe_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Probe_ProcessorInjector_UpdateTransformAndDebug::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    auto SpatialQuerySubsystem = GetWorld()->GetSubsystem<UCk_SpatialQuery_Subsystem>();
    if (ck::Is_NOT_Valid(SpatialQuerySubsystem))
    { return; }

    InWorld.Add<ck::FProcessor_Probe_UpdateTransform>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_Probe_DebugDraw>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Probe_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
{
    auto SpatialQuerySubsystem = GetWorld()->GetSubsystem<UCk_SpatialQuery_Subsystem>();
    if (ck::Is_NOT_Valid(SpatialQuerySubsystem))
    { return; }

    InWorld.Add<ck::FProcessor_Probe_Teardown>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
}

// --------------------------------------------------------------------------------------------------------------------
