#pragma once

#include <Fonts/SlateFontInfo.h>
#include <Widgets/SCompoundWidget.h>

// --------------------------------------------------------------------------------------------------------------------
// One key : value entry in an SCkWatermarkInfoBar.
// Value is a TAttribute so it can be live (evaluated each Slate paint pass).
// Visibility drives both the entry and its leading separator — set to Collapsed
// to suppress an entry without leaving an orphaned delimiter.

struct FCkWatermarkInfoBarEntry
{
    // Static label shown before the key-value separator (e.g. "CPU").
    FText Key;

    // Evaluated each paint pass — return the display string for this entry.
    TAttribute<FText> Value;

    // Drives both this entry and the separator that precedes it.
    TAttribute<EVisibility> Visibility = EVisibility::SelfHitTestInvisible;

    // Optional per-entry value color. When unset the bar's DefaultValueColor is used.
    TOptional<TAttribute<FSlateColor>> ValueColorOverride;
};

// --------------------------------------------------------------------------------------------------------------------
// Horizontal compact info bar.
//
// Renders entries as:   Key: Value  |  Key: Value  |  Key: Value
//
// Visual contract:
//   - KeyColor     — the label before the separator (e.g. gray)
//   - ValueColor   — the value text after the separator (e.g. white)
//   - SeparatorColor — both the inter-item delimiter and the key:value delimiter
//
// Separators between items share the visibility of their following entry so that
// collapsing an entry never leaves a trailing or orphaned pipe.

class CKWATERMARK_API SCkWatermarkInfoBar : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkWatermarkInfoBar)
        : _Separator(FText::FromString(TEXT("  |  ")))
        , _KeyValueSeparator(FText::FromString(TEXT(": ")))
        , _KeyColor(FLinearColor(0.55f, 0.55f, 0.55f, 1.f))
        , _ValueColor(FLinearColor(1.f, 1.f, 1.f, 1.f))
        , _SeparatorColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.f))
    {}
        // Ordered list of entries to render.
        SLATE_ARGUMENT(TArray<FCkWatermarkInfoBarEntry>, Entries)

        // Font applied to all text in the bar.
        SLATE_ARGUMENT(FSlateFontInfo, Font)

        // String placed between consecutive entries (default "  |  ").
        SLATE_ARGUMENT(FText, Separator)

        // String placed between a key and its value (default ": ").
        SLATE_ARGUMENT(FText, KeyValueSeparator)

        // Color of each entry's key text.
        SLATE_ARGUMENT(FLinearColor, KeyColor)

        // Color of each entry's value text.
        SLATE_ARGUMENT(FLinearColor, ValueColor)

        // Color of both inter-item and key:value separators.
        SLATE_ARGUMENT(FLinearColor, SeparatorColor)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
