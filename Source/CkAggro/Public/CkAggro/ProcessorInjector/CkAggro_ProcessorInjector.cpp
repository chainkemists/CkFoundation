#include "CkAggro_ProcessorInjector.h"

#include "CkAggro/CkAggro_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Aggro_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Aggro_DistanceScore>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Aggro_LineOfSightScore>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
