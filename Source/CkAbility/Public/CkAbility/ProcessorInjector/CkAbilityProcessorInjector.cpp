#include "CkEcsProcessorInjector.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"
#include "CkEcs/OwningActor/CkOwningActor_Processors.h"

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
}

// --------------------------------------------------------------------------------------------------------------------
