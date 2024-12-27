#include "CkProbe_ProcessorInjector.h"

#include "CkSpatialQuery/Probe/CkProbe_Processor.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Probe_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    auto PhysicsWorld = GEngine->GetEngineSubsystem<UCk_SpatialQuery_Subsystem_UE>()->Get_PhysicsWorld();

    InWorld.Add<ck::FProcessor_Probe_Setup>(InWorld.Get_Registry(), PhysicsWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Probe_UpdateTransform>(InWorld.Get_Registry(), PhysicsWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Probe_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Probe_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Probe_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
