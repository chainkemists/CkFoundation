#include "CkAggro/ProcessorInjector/CkAggroOwner_ProcessorInjector.h"

#include "CkAggro/CkAggroOwner_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AggroOwner_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Aggro_DistanceScore>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Aggro_LineOfSightScore>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Aggro_UpdateBestAggro>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
