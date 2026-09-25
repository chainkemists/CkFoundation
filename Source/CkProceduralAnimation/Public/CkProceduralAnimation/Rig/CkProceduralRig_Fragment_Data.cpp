#include "CkProceduralAnimation/Rig/CkProceduralRig_Fragment_Data.h"

#include "CkCore/Format/CkFormat.h"

#if WITH_EDITOR
#include <Misc/DataValidation.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ProceduralRig_Data::
    IsDataValid(
        FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (_Legs.Num() < 2 || _Legs.Num() > 64)
    {
        InContext.AddError(FText::FromString(ck::Format_UE(
            TEXT("UCk_ProceduralRig_Data needs 2..64 legs; it has [{}]."), _Legs.Num())));
        Result = EDataValidationResult::Invalid;
    }

    auto Ids = TSet<FName>{};
    for (auto Index = 0; Index < _Legs.Num(); ++Index)
    {
        const auto& Leg = _Legs[Index];

        if (ck::Is_NOT_Valid(Leg))
        {
            InContext.AddError(FText::FromString(ck::Format_UE(
                TEXT("Leg [{}] ({}) is invalid: it needs an Id, a finite placement with phase offset in [0, 1) and a "
                     "positive step threshold scale, and 1..8 positive segment lengths with a finite pole."),
                Index, Leg.Get_Id())));
            Result = EDataValidationResult::Invalid;
        }

        if (Ids.Contains(Leg.Get_Id()))
        {
            InContext.AddError(FText::FromString(ck::Format_UE(
                TEXT("Leg [{}] reuses the Id ({}); leg Ids must be unique."), Index, Leg.Get_Id())));
            Result = EDataValidationResult::Invalid;
        }

        Ids.Add(Leg.Get_Id());
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
