#include "CkProbe_ProcessorInjector.h"

#include "CkSpatialQuery/Probe/CkProbe_Processor.h"
#include "CkSpatialQuery/Probe/CkProbeTrace_Processor.h"
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
    InWorld.Add<ck::FProcessor_Probe_HandleRequests>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_Probe_UpdateShape>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_ProbeTrace_RayCast>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_ProbeTrace_ShapeCast>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
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
    
#if NOT CK_BUILD_TEST_OR_SHIPPING
    InWorld.Add<ck::FProcessor_Probe_EnsureStaticNotMoved_DEBUG>(InWorld.Get_Registry());
#endif

    InWorld.Add<ck::FProcessor_Probe_UpdateTransform>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_Probe_UpdateTransform_LinearCast>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_Probe_DebugDraw>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Probe_DebugDrawAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ProbeTrace_DebugDraw_RayCast>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
    InWorld.Add<ck::FProcessor_ProbeTrace_DebugDraw_ShapeCast>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Probe_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    auto SpatialQuerySubsystem = GetWorld()->GetSubsystem<UCk_SpatialQuery_Subsystem>();
    if (ck::Is_NOT_Valid(SpatialQuerySubsystem))
    { return; }

    InWorld.Add<ck::FProcessor_Probe_EndPlay>(InWorld.Get_Registry(), SpatialQuerySubsystem->Get_PhysicsSystem());
}

// --------------------------------------------------------------------------------------------------------------------
