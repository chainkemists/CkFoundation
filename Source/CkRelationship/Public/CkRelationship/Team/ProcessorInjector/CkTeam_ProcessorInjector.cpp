#include "CkTeam_ProcessorInjector.h"

#include "CkRelationship/Team/CkTeam_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Team_ProcessorInjector_Setup_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_TeamAssignedSignal_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_OnTeamAssigned_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
