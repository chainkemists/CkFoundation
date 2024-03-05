#include "CkActorProcessorInjector.h"

#include "CkActor/ActorModifier/CkActorModifier_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Actor_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ActorModifier_SpawnActor_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ActorModifier_AddActorComponent_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ActorModifier_RemoveActorComponent_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
