#include "CkSubstep_ProcessorInjector.h"

#include "CkSubstep/CkSubstep_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Substep_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Substep_Update>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
