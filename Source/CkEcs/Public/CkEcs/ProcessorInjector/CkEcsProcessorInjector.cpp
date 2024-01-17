#include "CkEcsProcessorInjector.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"
#include "CkEcs/OwningActor/CkOwningActor_Processors.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorInjector_EntityDestruction::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_EntityLifetime_TriggerDestroyEntity>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_OwningActor_Destroy>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityLifetime_EntityJustCreated>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityLifetime_PendingDestroyEntity>(InWorld.Get_Registry());

#if CK_MEMORY_TRACKING
    InWorld.Add<ck::FProcessor_Memory_Stats>();
#endif
}

// --------------------------------------------------------------------------------------------------------------------
