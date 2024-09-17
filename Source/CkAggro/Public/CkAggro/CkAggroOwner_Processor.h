#pragma once

#include "CkAggro_Fragment.h"

#include "CkAggro/CkAggroOwner_Fragment.h"
#include "CkAggro/CkAggroOwner_Fragment_Data.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_Aggro_DistanceScore : public ck_exp::TProcessor<
        FProcessor_Aggro_DistanceScore,
        FCk_Handle_AggroOwner,
        FFragment_AggroOwner_Params,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AggroOwner_Params& InParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_Aggro_LineOfSightScore : public ck_exp::TProcessor<
        FProcessor_Aggro_LineOfSightScore,
        FCk_Handle_AggroOwner,
        FFragment_AggroOwner_Current,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AggroOwner_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}
