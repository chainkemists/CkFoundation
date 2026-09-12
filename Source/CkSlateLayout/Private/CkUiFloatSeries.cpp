#include "CkSlateLayout/CkUiFloatSeries.h"

namespace ck_ui_float_series
{
    auto Failure(const FString& InError) -> FCkUiLoadResult
    {
        return FCkUiLoadResult{.Succeeded = false, .Errors = {InError}};
    }

    auto HasOnlyFiniteSamples(const TArray<float>& InSamples) -> bool
    {
        for (const float Sample : InSamples)
        {
            if (!FMath::IsFinite(Sample)) { return false; }
        }
        return true;
    }
}

auto FCkUiFloatSeries::TryCreate(TArray<float> InSamples, TSharedPtr<FCkUiFloatSeries>& OutSeries) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_float_series::Failure(TEXT("float series creation must run on the game thread")); }
    if (!ck_ui_float_series::HasOnlyFiniteSamples(InSamples)) { return ck_ui_float_series::Failure(TEXT("float series samples must be finite")); }
    OutSeries = MakeShareable(new FCkUiFloatSeries(MoveTemp(InSamples)));
    return FCkUiLoadResult{.Succeeded = true};
}

auto FCkUiFloatSeries::TrySetSamples(TArray<float> InSamples) -> FCkUiLoadResult
{
    if (!IsInGameThread()) { return ck_ui_float_series::Failure(TEXT("float series mutation must run on the game thread")); }
    if (!ck_ui_float_series::HasOnlyFiniteSamples(InSamples)) { return ck_ui_float_series::Failure(TEXT("float series samples must be finite")); }
    _Samples = MoveTemp(InSamples);
    ++_Revision;
    return FCkUiLoadResult{.Succeeded = true};
}
