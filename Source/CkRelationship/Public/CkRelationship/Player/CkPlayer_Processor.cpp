#include "CkPlayer_Processor.h"

#include "CkRelationship/CkRelationship_Log.h"
#include "CkRelationship/Player/CkPlayer_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Player_RetryPendingReplication);

namespace
ck
{
    auto
        FProcessor_Player_RetryPendingReplication::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Player_PendingReplication& InPending) const
        -> void
    {
        if (NOT UCk_Utils_Player_UE::Has(InHandle))
        { return; }

        auto PlayerEntity = UCk_Utils_Player_UE::Cast(InHandle);

        if (NOT UCk_Utils_Player_UE::Get_IsAssignedTo(PlayerEntity, InPending.Get_PlayerID()))
        {
            ck::relationship::Verbose(TEXT("Applying pending replicated Player ID [{}] to Entity [{}]"),
                InPending.Get_PlayerID(), InHandle);

            UCk_Utils_Player_UE::Assign(PlayerEntity, InPending.Get_PlayerID());
        }

        InHandle.Remove<FFragment_Player_PendingReplication>();
    }
}
