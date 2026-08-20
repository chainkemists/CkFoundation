#include "CkPixelArt/CkPixelArt_Params.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PixelArt_PreconditionReport::
    ToText() const
    -> FString
{
    if (_Failures.IsEmpty())
    { return TEXT("all preconditions satisfied"); }

    const auto Lines = ck::algo::Transform<TArray<FString>>(_Failures,
    [](const FCk_PixelArt_PreconditionFailure& InFailure) -> FString
    {
        return ck::Format_UE(TEXT("{} is [{}], needs [{}] — fix with `{}`"),
            InFailure.Get_Name(), InFailure.Get_CurrentValue(), InFailure.Get_RequiredValue(),
            InFailure.Get_FixCVar());
    });

    return FString::Join(Lines, TEXT("; "));
}
