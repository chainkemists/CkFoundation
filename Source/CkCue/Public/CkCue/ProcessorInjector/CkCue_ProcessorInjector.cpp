#include "CkCue_ProcessorInjector.h"

#include "CkCue/CkCue_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Cue_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Cue_Execute>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Cue_ExecuteLocal>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------