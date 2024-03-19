#pragma once

#include "CkSignal/CkSignal_Macros.h"

#include "CkSubstep/CkSubstep_Fragment_Data.h"

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Substep_FirstUpdate);
    CK_DEFINE_ECS_TAG(FTag_Substep_Update);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSUBSTEP_API FFragment_Substep_Params
    {
    public:
        using ParamsType = FCk_Substep_ParamsData;

    private:
        ParamsType _Data;

    public:
        CK_PROPERTY_GET(_Data);

        CK_DEFINE_CONSTRUCTORS(FFragment_Substep_Params, _Data);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSUBSTEP_API FFragment_Substep_Current
    {
        friend class FProcessor_Substep_Update;

    private:
        FCk_Time _DeltaOverflowFromLastFrame;

    public:
        CK_PROPERTY_GET(_DeltaOverflowFromLastFrame);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSUBSTEP_API, OnSubstepFirstUpdate, FCk_Delegate_Substep_OnFirstUpdate_MC,
        FCk_Handle_Substep, FCk_Time);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSUBSTEP_API, OnSubstepUpdate, FCk_Delegate_Substep_OnUpdate_MC,
        FCk_Handle_Substep, FCk_Time, int32, FCk_Time);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSUBSTEP_API, OnSubstepFrameEnd, FCk_Delegate_Substep_OnFrameEnd_MC,
        FCk_Handle_Substep, FCk_Time);
}
