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
        FTag_Aggro_Filter_Distance,
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
        FTag_Aggro_Filter_LoS,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKAGGRO_API FProcessor_Aggro_UpdateBestAggro : public ck_exp::TProcessor<
        FProcessor_Aggro_UpdateBestAggro,
        FCk_Handle_AggroOwner,
        FFragment_AggroOwner_Current,
        FFragment_AggroOwner_NewBestAggro,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AggroOwner_Current& InCurrent,
            const FFragment_AggroOwner_NewBestAggro& InNewBestAggro) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}
