#include "CkPixelArt/CkPixelArt_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PixelArt_PreconditionReport::
    ToText() const
    -> FString
{
    if (_Failures.IsEmpty())
    { return TEXT("all preconditions satisfied"); }

    auto Lines = TArray<FString>{};
    Lines.Reserve(_Failures.Num());

    for (const auto& Failure : _Failures)
    {
        Lines.Add(FString::Printf(TEXT("%s is [%s], needs [%s] — fix with `%s`"),
            *Failure.Get_Name(), *Failure.Get_CurrentValue(), *Failure.Get_RequiredValue(),
            *Failure.Get_FixCVar()));
    }

    return FString::Join(Lines, TEXT("; "));
}
