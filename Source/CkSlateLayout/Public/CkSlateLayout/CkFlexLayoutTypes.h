#pragma once

#include "CoreMinimal.h"
#include "Types/ISlateMetaData.h"
#include <yoga/Yoga.h>

struct FCkFlexMeasureArgs
{
    float AvailableWidth = YGUndefined;
    YGMeasureMode WidthMode = YGMeasureModeUndefined;
    float AvailableHeight = YGUndefined;
    YGMeasureMode HeightMode = YGMeasureModeUndefined;
    float LayoutScale = 1.0f;
};

inline auto IsValid_CkFlexMeasureArgs(const FCkFlexMeasureArgs& InArgs) -> bool
{
    const auto IsValidMode = [](const YGMeasureMode InMode) -> bool
    {
        return InMode == YGMeasureModeUndefined || InMode == YGMeasureModeExactly || InMode == YGMeasureModeAtMost;
    };
    const auto IsValidDimension = [&IsValidMode](const float InDimension, const YGMeasureMode InMode) -> bool
    {
        return IsValidMode(InMode) && (InMode == YGMeasureModeUndefined || (FMath::IsFinite(InDimension) && InDimension >= 0.0f));
    };
    return IsValidDimension(InArgs.AvailableWidth, InArgs.WidthMode)
        && IsValidDimension(InArgs.AvailableHeight, InArgs.HeightMode)
        && FMath::IsFinite(InArgs.LayoutScale) && InArgs.LayoutScale > 0.0f;
}

class FCkFlexMeasureMetaData final : public ISlateMetaData
{
public:
    SLATE_METADATA_TYPE(FCkFlexMeasureMetaData, ISlateMetaData)

    using FMeasure = TFunction<FVector2D(const FCkFlexMeasureArgs&)>;
    using FOnArranged = TFunction<void(float, float)>;

    FCkFlexMeasureMetaData(FMeasure InMeasure, FOnArranged InOnArranged)
        : _Measure(MoveTemp(InMeasure))
        , _OnArranged(MoveTemp(InOnArranged))
    {
    }

    FVector2D Measure(const FCkFlexMeasureArgs& InArgs) const
    {
        return _Measure ? _Measure(InArgs) : FVector2D::ZeroVector;
    }

    void NotifyArranged(const float InWidth, const float InHeight) const
    {
        if (_OnArranged)
        {
            _OnArranged(InWidth, InHeight);
        }
    }

private:
    FMeasure _Measure;
    FOnArranged _OnArranged;
};
