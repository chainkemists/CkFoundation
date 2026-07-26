#pragma once

#include <Fonts/SlateFontInfo.h>
#include <Widgets/SCompoundWidget.h>

// --------------------------------------------------------------------------------------------------------------------

struct FCkWatermarkInfoBarEntry
{
    TAttribute<FText> Key;

    TAttribute<FText> Value;

    // Drives both this entry and the separator that precedes it.
    TAttribute<EVisibility> Visibility = EVisibility::SelfHitTestInvisible;

    TOptional<TAttribute<FSlateColor>> KeyColorOverride;

    TOptional<TAttribute<FSlateColor>> ValueColorOverride;
};

// --------------------------------------------------------------------------------------------------------------------
// Renders:   Key: Value  |  Key: Value.  SeparatorColor covers the inter-item AND key:value delimiters.

class CKWATERMARK_API SCkWatermarkInfoBar : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkWatermarkInfoBar)
        : _Separator(FText::FromString(TEXT("  |  ")))
        , _KeyValueSeparator(FText::FromString(TEXT(": ")))
        , _KeyColor(FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f, 1.f)))
        , _ValueColor(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f)))
        , _SeparatorColor(FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.f)))
        , _ShadowOffset(FVector2D::ZeroVector)
        , _ShadowColorAndOpacity(FLinearColor::Black)
    {}
        SLATE_ARGUMENT(TArray<FCkWatermarkInfoBarEntry>, Entries)

        SLATE_ATTRIBUTE(FSlateFontInfo, Font)

        SLATE_ARGUMENT(FText, Separator)

        SLATE_ARGUMENT(FText, KeyValueSeparator)

        SLATE_ATTRIBUTE(FSlateColor, KeyColor)

        SLATE_ATTRIBUTE(FSlateColor, ValueColor)

        SLATE_ATTRIBUTE(FSlateColor, SeparatorColor)

        SLATE_ATTRIBUTE(FVector2D,    ShadowOffset)

        SLATE_ATTRIBUTE(FLinearColor, ShadowColorAndOpacity)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
