#pragma once

#include "CkWatermark/CkWatermark_ActivityBar_Types.h"

#include <Fonts/SlateFontInfo.h>
#include <Widgets/SCompoundWidget.h>
#include <Widgets/SBoxPanel.h>

class STextBlock;

// --------------------------------------------------------------------------------------------------------------------
// Horizontal activity bar rendered in the watermark stats group.
//
// Displays a row of signal chips:  [ W ] [ A ] [ SPACE ] [ LMB ]
//
// Active signals render in the ActiveColor.
// Held signals (active for more than 1 frame) render in the HeldAccentColor
// and show a colored underline accent bar.
// Inactive (released) signals render in InactiveColor and appear on the left.
//
// Chip widgets are pre-allocated and updated in place (SetText / SetVisibility)
// to avoid layout flash from destroying and recreating STextBlock widgets.

class CKWATERMARK_API SCkWatermarkActivityBar : public SCompoundWidget
{
public:
    using FActivityGetter = TFunction<TPair<uint32, const TArray<FCkWatermarkActivityState>*>()>;

    SLATE_BEGIN_ARGS(SCkWatermarkActivityBar)
        : _BracketOpen(FText::FromString(TEXT("[")))
        , _BracketClose(FText::FromString(TEXT("]")))
        , _BracketInnerPadding(3.f)
        , _ChipSpacing(2.f)
        , _HeldAccentHeight(2.f)
        , _ActiveColor(FSlateColor(FLinearColor::White))
        , _HeldAccentColor(FLinearColor(1.0f, 0.7f, 0.1f, 1.0f))
        , _InactiveColor(FSlateColor(FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)))
        , _ShadowOffset(FVector2D::ZeroVector)
        , _ShadowColorAndOpacity(FLinearColor::Black)
    {}
        // Callable that returns (version, activity array ptr). Called each Tick.
        SLATE_ARGUMENT(FActivityGetter, ActivityGetter)

        SLATE_ATTRIBUTE(FSlateFontInfo, Font)

        SLATE_ARGUMENT(FText, BracketOpen)
        SLATE_ARGUMENT(FText, BracketClose)
        SLATE_ARGUMENT(float, BracketInnerPadding)

        // Horizontal spacing between adjacent chips.
        SLATE_ARGUMENT(float, ChipSpacing)

        // Height of the accent underline bar shown under held signals.
        SLATE_ARGUMENT(float, HeldAccentHeight)

        SLATE_ATTRIBUTE(FSlateColor,   ActiveColor)
        SLATE_ATTRIBUTE(FLinearColor,   HeldAccentColor)
        SLATE_ATTRIBUTE(FSlateColor,   InactiveColor)
        SLATE_ATTRIBUTE(FVector2D,     ShadowOffset)
        SLATE_ATTRIBUTE(FLinearColor,  ShadowColorAndOpacity)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
    auto Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) -> void override;

private:
    auto DoUpdateChips(const TArray<FCkWatermarkActivityState>& InStates) -> void;
    auto DoEnsurePoolSize(int32 InRequired) -> void;

    // One pre-allocated chip entry in the pool.
    struct FChipSlot
    {
        TSharedPtr<SWidget>    Root;         // SVerticalBox — visibility for pool management.
        TSharedPtr<STextBlock> LabelBlock;   // "[ LMB ]"
        TSharedPtr<STextBlock> CounterBlock; // "142f"
        TSharedPtr<SWidget>    AccentBar;    // SBox — visibility for held underline.
    };

private:
    FActivityGetter              _ActivityGetter;
    TSharedPtr<SHorizontalBox>   _HBox;
    uint32                       _CachedVersion = 0;

    TArray<FChipSlot>            _ChipPool;

    // Captured from Construct.
    TAttribute<FSlateFontInfo> _Font;
    FText                      _BracketOpen;
    FText                      _BracketClose;
    float                      _BracketInnerPadding = 3.f;
    float                      _ChipSpacing         = 2.f;
    float                      _HeldAccentHeight    = 2.f;
    TAttribute<FSlateColor>    _ActiveColor;
    TAttribute<FLinearColor>   _HeldAccentColor;
    TAttribute<FSlateColor>    _InactiveColor;
    TAttribute<FVector2D>      _ShadowOffset;
    TAttribute<FLinearColor>   _ShadowColorAndOpacity;
};

// --------------------------------------------------------------------------------------------------------------------
