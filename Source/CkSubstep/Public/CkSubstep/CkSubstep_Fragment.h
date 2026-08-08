#pragma once

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkSubstep/CkSubstep_Fragment_Data.h"

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Substep_FirstUpdate);
    CK_DEFINE_ECS_TAG(FTag_Substep_Update);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Substep_Params = FCk_Substep_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSUBSTEP_API FFragment_Substep
    {
        friend class FProcessor_Substep_Update;

        CK_GENERATED_BODY(FFragment_Substep);

    private:
        FCk_Time _DeltaOverflowFromLastFrame;

    public:
        CK_PROPERTY_GET(_DeltaOverflowFromLastFrame);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSUBSTEP_API, OnSubstepFirstUpdate, FCk_Delegate_Substep_OnFirstUpdate,
        FCk_Handle_Substep, FCk_Time);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSUBSTEP_API, OnSubstepUpdate, FCk_Delegate_Substep_OnUpdate,
        FCk_Handle_Substep, FCk_Time, int32, FCk_Time);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSUBSTEP_API, OnSubstepFrameEnd, FCk_Delegate_Substep_OnFrameEnd,
        FCk_Handle_Substep, FCk_Time);
}
