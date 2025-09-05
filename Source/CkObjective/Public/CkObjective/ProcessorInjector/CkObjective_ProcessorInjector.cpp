#include "CkObjective_ProcessorInjector.h"

#include "CkObjective/Objective/CkObjective_Processor.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Objective_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Objective_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Objective_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ObjectiveOwner_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ObjectiveOwner_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Objective_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------