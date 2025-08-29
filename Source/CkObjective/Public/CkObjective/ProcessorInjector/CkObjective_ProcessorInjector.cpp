#include "CkObjective_ProcessorInjector.h"

#include "CkObjective/Objective/CkObjective_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Objective_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Objective_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------