#pragma once

#include "CkEcs/Processor/CkProcessor.h"

#include "CkSubstep/CkSubstep_Fragment.h"
#include "CkSubstep/CkSubstep_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKSUBSTEP_API FProcessor_Substep_Update
        : public ck_exp::TProcessor<FProcessor_Substep_Update, FCk_Handle_Substep,
            FFragment_Substep_Params, FFragment_Substep_Current, FTag_Substep_Update, CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle_Substep& InHandle,
            const FFragment_Substep_Params& InParams,
            FFragment_Substep_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}
