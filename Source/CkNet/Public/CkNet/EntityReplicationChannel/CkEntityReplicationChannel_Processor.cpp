#include "CkEntityReplicationChannel_Processor.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkNet/EntityReplicationChannel/CkEntityReplicationChannel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityReplicationChannel_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EntityReplicationChannel_Setup::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActor)
        -> void
    {
        UCk_Utils_Handle_UE::Set_DebugName(InHandle, *InOwningActor.Get_EntityOwningActor()->GetActorLabel());

        auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle);
        UCk_Utils_EntityReplicationChannelOwner_UE::AddNewChannel(TransientEntity, InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
