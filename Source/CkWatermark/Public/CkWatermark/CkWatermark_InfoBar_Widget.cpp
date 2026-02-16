#include "CkWatermark_InfoBar_Widget.h"

#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

auto SCkWatermarkInfoBar::Construct(const FArguments& InArgs) -> void
{
    const TAttribute<FSlateFontInfo> Font_In     = InArgs._Font;
    const FText                   Sep            = InArgs._Separator;
    const FText                   KVSep          = InArgs._KeyValueSeparator;
    const TAttribute<FSlateColor> KeyCol         = InArgs._KeyColor;
    const TAttribute<FSlateColor> ValCol         = InArgs._ValueColor;
    const TAttribute<FSlateColor> SepCol         = InArgs._SeparatorColor;
    const TAttribute<FVector2D>    ShadowOff     = InArgs._ShadowOffset;
    const TAttribute<FLinearColor> ShadowColor   = InArgs._ShadowColorAndOpacity;

    // Wrap each font so its outline alpha tracks the paired text color's alpha.
    auto WithTextAlpha = [](TAttribute<FSlateFontInfo> InFont, TAttribute<FSlateColor> InColor) -> TAttribute<FSlateFontInfo>
    {
        return TAttribute<FSlateFontInfo>::CreateLambda([InFont, InColor]() -> FSlateFontInfo
        {
            FSlateFontInfo F = InFont.Get();
            const FSlateColor SC = InColor.Get(FSlateColor(FLinearColor::White));
            if (SC.IsColorSpecified())
            {
                F.OutlineSettings.OutlineColor.A *= SC.GetSpecifiedColor().A;
            }
            return F;
        });
    };

    const TAttribute<FSlateFontInfo> KeyFont = WithTextAlpha(Font_In, KeyCol);
    const TAttribute<FSlateFontInfo> ValFont = WithTextAlpha(Font_In, ValCol);
    const TAttribute<FSlateFontInfo> SepFont = WithTextAlpha(Font_In, SepCol);

    TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);

    bool bFirst = true;
    for (const FCkWatermarkInfoBarEntry& Entry : InArgs._Entries)
    {
        const TAttribute<EVisibility> EntryVis = Entry.Visibility;

        // Inter-item separator — shares the visibility of the entry it precedes.
        // Omitted before the first entry.
        if (!bFirst)
        {
            Box->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(Sep)
                    .Font(SepFont)
                    .ColorAndOpacity(SepCol)
                    .ShadowOffset(ShadowOff)
                    .ShadowColorAndOpacity(ShadowColor)
                    .Visibility(EntryVis)
                ];
        }
        bFirst = false;

        // Key:Value pair — collapses as a unit.
        const FText CapturedKey = Entry.Key;
        Box->AddSlot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(EntryVis)

                // Key
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(CapturedKey)
                    .Font(KeyFont)
                    .ColorAndOpacity(KeyCol)
                    .ShadowOffset(ShadowOff)
                    .ShadowColorAndOpacity(ShadowColor)
                ]

                // Key:Value separator
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(KVSep)
                    .Font(SepFont)
                    .ColorAndOpacity(SepCol)
                    .ShadowOffset(ShadowOff)
                    .ShadowColorAndOpacity(ShadowColor)
                ]

                // Value — uses per-entry color override if set, else bar default.
                // For override colors, derive a font with the override's alpha applied to outline.
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(Entry.Value)
                    .Font(Entry.ValueColorOverride.IsSet()
                        ? WithTextAlpha(Font_In, Entry.ValueColorOverride.GetValue())
                        : ValFont)
                    .ColorAndOpacity(Entry.ValueColorOverride.IsSet()
                        ? Entry.ValueColorOverride.GetValue()
                        : TAttribute<FSlateColor>(ValCol))
                    .ShadowOffset(ShadowOff)
                    .ShadowColorAndOpacity(ShadowColor)
                ]
            ];
    }

    ChildSlot [ Box ];
}

// --------------------------------------------------------------------------------------------------------------------
