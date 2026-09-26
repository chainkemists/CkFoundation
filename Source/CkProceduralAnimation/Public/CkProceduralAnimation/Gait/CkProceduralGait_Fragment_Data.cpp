#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"

#if WITH_EDITOR
#include <Misc/DataValidation.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ProceduralGait_Data::
    IsDataValid(
        FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (ck::Is_NOT_Valid(_Timing))
    {
        InContext.AddError(FText::FromString(TEXT("FCk_ProceduralGait_Timing is invalid: cycle and step durations must be finite and "
            "positive, the cadence reference >= 0, the max cadence scale >= 1 and the max simultaneous swings >= 0.")));
        Result = EDataValidationResult::Invalid;
    }

    if (ck::Is_NOT_Valid(_Step))
    {
        InContext.AddError(FText::FromString(TEXT("FCk_ProceduralGait_Step is invalid: height, obstacle clearance and max velocity "
            "lead must be finite and >= 0, the threshold finite and > 0, and the reach fractions finite with "
            "0 < TargetReachFraction < ForceStepReachFraction <= 1 and ForceStepReachFraction < HardOverstretchReachFraction, "
            "which lies in [1, 1.5].")));
        Result = EDataValidationResult::Invalid;
    }

    if (ck::Is_NOT_Valid(_Probe))
    {
        InContext.AddError(FText::FromString(TEXT("FCk_ProceduralGait_Probe is invalid: up and down distances must be finite and > 0, "
            "the outward lean in [0, 1] and the contact grace finite and >= 0.")));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
