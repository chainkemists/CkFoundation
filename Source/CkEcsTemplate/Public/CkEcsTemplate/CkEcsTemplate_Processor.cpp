#include "CkEcsTemplate_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsTemplate/CkEcsTemplate_Log.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EcsTemplate_HandleRequests::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_EcsTemplate_Updated>();

        TProcessor::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_EcsTemplate_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EcsTemplate_Current& InComp,
            FFragment_EcsTemplate_Requests& InRequestsComp) const
        -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EcsTemplate_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EcsTemplate_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_EcsTemplate_Rep>& InComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::UpdateReplicatedFragment<UCk_Fragment_EcsTemplate_Rep>(InHandle, [&](UCk_Fragment_EcsTemplate_Rep* InRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
