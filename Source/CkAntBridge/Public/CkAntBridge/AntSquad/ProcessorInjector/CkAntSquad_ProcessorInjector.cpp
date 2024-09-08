#include "CkAntSquad_ProcessorInjector.h"

#include "CkAntBridge/AntSquad/CkAntSquad_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AntSquad_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AntSquad_HandleRequests>(InWorld.Get_Registry(), GetWorld());
    InWorld.Add<ck::FProcessor_AntSquad_UpdateInstances>(InWorld.Get_Registry(), GetWorld());
}

// --------------------------------------------------------------------------------------------------------------------
