#include "CkWatermark_InfoBar_Widget.h"

#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

auto SCkWatermarkInfoBar::Construct(const FArguments& InArgs) -> void
{
    const TAttribute<FSlateFontInfo> Font   = InArgs._Font;
    const FText                   Sep      = InArgs._Separator;
    const FText                   KVSep    = InArgs._KeyValueSeparator;
    const TAttribute<FSlateColor> KeyCol   = InArgs._KeyColor;
    const TAttribute<FSlateColor> ValCol   = InArgs._ValueColor;
    const TAttribute<FSlateColor> SepCol   = InArgs._SeparatorColor;

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
                    .Font(Font)
                    .ColorAndOpacity(SepCol)
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
                    .Font(Font)
                    .ColorAndOpacity(KeyCol)
                ]

                // Key:Value separator
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(KVSep)
                    .Font(Font)
                    .ColorAndOpacity(SepCol)
                ]

                // Value — uses per-entry color override if set, else bar default
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock)
                    .Text(Entry.Value)
                    .Font(Font)
                    .ColorAndOpacity(Entry.ValueColorOverride.IsSet()
                        ? Entry.ValueColorOverride.GetValue()
                        : TAttribute<FSlateColor>(ValCol))
                ]
            ];
    }

    ChildSlot [ Box ];
}

// --------------------------------------------------------------------------------------------------------------------
