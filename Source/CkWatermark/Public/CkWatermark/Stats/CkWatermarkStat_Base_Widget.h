#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/Widget.h>
#include <Styling/CoreStyle.h>
#include <Fonts/SlateFontInfo.h>
#include <Widgets/SCompoundWidget.h>

#include "CkWatermarkStat_Base_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Pure Slate widget — shared renderer for all stat types.
// Displays:  [ VALUE ]
//              name
// Value text and color are driven by TAttributes evaluated each frame.

class CKWATERMARK_API SCkWatermarkStat : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkWatermarkStat)
        : _StatName(FText::GetEmpty())
        , _ValueFont(FCoreStyle::GetDefaultFontStyle("Bold", 11))
        , _LabelFont(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        , _BracketFont(FCoreStyle::GetDefaultFontStyle("Bold", 11))
        , _BracketOpen(FText::FromString(TEXT("[")))
        , _BracketClose(FText::FromString(TEXT("]")))
        , _BracketInnerPadding(3.f)
        , _LabelColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
    {}
        SLATE_ATTRIBUTE(FText,         StatValue)
        SLATE_ATTRIBUTE(FSlateColor,   ValueColor)
        SLATE_ARGUMENT(FText,          StatName)
        SLATE_ARGUMENT(FSlateFontInfo, ValueFont)
        SLATE_ARGUMENT(FSlateFontInfo, LabelFont)
        SLATE_ARGUMENT(FSlateFontInfo, BracketFont)
        SLATE_ARGUMENT(FText,          BracketOpen)
        SLATE_ARGUMENT(FText,          BracketClose)
        SLATE_ARGUMENT(float,          BracketInnerPadding)
        SLATE_ARGUMENT(FLinearColor,   LabelColor)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// UWidget base — subclass once per stat type, override the three Native* virtuals.
// RebuildWidget() wires the TAttribute lambdas to the Slate widget automatically.

UCLASS(Abstract, BlueprintType, Blueprintable,
       meta = (DisplayName = "Ck Watermark Stat (Base)",
               Category = "CkFoundation|Watermark Stats"))
class CKWATERMARK_API UCkWatermarkStat_Base_UWidget_UE : public UWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkWatermarkStat_Base_UWidget_UE);

public:
    UCkWatermarkStat_Base_UWidget_UE();

    // ---------------- override in each stat subclass ----------------
    virtual auto NativeGetStatName()  const -> FText         { return FText::GetEmpty(); }
    virtual auto NativeGetStatValue() const -> FText         { return FText::GetEmpty(); }
    virtual auto NativeGetStatColor() const -> FLinearColor  { return FLinearColor::White; }
    // ----------------------------------------------------------------

protected:
    // UWidget interface
    auto RebuildWidget()                      -> TSharedRef<SWidget> override;
    auto ReleaseSlateResources(bool)          -> void                override;
    auto SynchronizeProperties()              -> void                override;
#if WITH_EDITOR
    auto GetPaletteCategory()                 -> const FText         override;
#endif

protected:
    TSharedPtr<SCkWatermarkStat> _SlateWidget;
};

// --------------------------------------------------------------------------------------------------------------------
