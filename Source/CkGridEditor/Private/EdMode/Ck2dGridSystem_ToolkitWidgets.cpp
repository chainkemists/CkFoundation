#include "CkGridEditor/EdMode/Ck2dGridSystem_ToolkitWidgets.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::grid_paint_style
{
    auto BgRoot()   -> FLinearColor { return FLinearColor(FColor(0x0b, 0x0e, 0x13, 255)); }
    auto Bg1()      -> FLinearColor { return FLinearColor(FColor(0x11, 0x15, 0x1c, 255)); }
    auto Bg2()      -> FLinearColor { return FLinearColor(FColor(0x16, 0x1b, 0x24, 255)); }
    auto Bg3()      -> FLinearColor { return FLinearColor(FColor(0x1b, 0x22, 0x30, 255)); }
    auto Border()   -> FLinearColor { return FLinearColor(FColor(0x23, 0x2a, 0x38, 255)); }
    auto Text()     -> FLinearColor { return FLinearColor(FColor(0xe5, 0xe7, 0xed, 255)); }
    auto TextDim()  -> FLinearColor { return FLinearColor(FColor(0x8a, 0x92, 0xa4, 255)); }
    auto TextMute() -> FLinearColor { return FLinearColor(FColor(0x5a, 0x62, 0x77, 255)); }
    auto Accent()   -> FLinearColor { return FLinearColor(FColor(0xf5, 0xc8, 0x42, 255)); }
    auto Info()     -> FLinearColor { return FLinearColor(FColor(0x5f, 0xb3, 0xd4, 255)); }

    auto Font_Bold(int32 InSize) -> FSlateFontInfo
    {
        return FCoreStyle::GetDefaultFontStyle("Bold", InSize);
    }

    auto Font_Regular(int32 InSize) -> FSlateFontInfo
    {
        return FCoreStyle::GetDefaultFontStyle("Regular", InSize);
    }

    auto WhiteBrush() -> const FSlateBrush*
    {
        // Flat square fill (no rounding) — matches the debugger's tint-via-color convention.
        return FAppStyle::Get().GetBrush("GenericWhiteBox");
    }

    auto
    Make_SectionHeader(
        const FText& InLabel) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .Padding(FMargin(0.0f, SpaceS, 0.0f, SpaceS))
            [
                SNew(STextBlock)
                .Text(FText::FromString(InLabel.ToString().ToUpper()))
                .Font(Font_Bold(9))
                .ColorAndOpacity(FSlateColor(TextDim()))
            ];
    }

    auto
    Make_LabeledGroup(
        const FText&            InTitle,
        TSharedRef<SWidget>     InBody,
        TOptional<FLinearColor> InDotColor) -> TSharedRef<SWidget>
    {
        auto HeaderRow = SNew(SHorizontalBox);

        if (InDotColor.IsSet())
        {
            HeaderRow->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, SpaceM, 0.0f)
                [
                    Make_Swatch(InDotColor.GetValue(), 8.0f)
                ];
        }

        HeaderRow->AddSlot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(InTitle.ToString().ToUpper()))
                .Font(Font_Bold(9))
                .ColorAndOpacity(FSlateColor(TextDim()))
            ];

        // Outer 1px border-colored card wrapping the header strip + body panel.
        return SNew(SBorder)
            .BorderImage(WhiteBrush())
            .BorderBackgroundColor(FSlateColor(Border()))
            .Padding(FMargin(1.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .BorderImage(WhiteBrush())
                    .BorderBackgroundColor(FSlateColor(Bg1()))
                    .Padding(FMargin(SpaceM, SpaceS))
                    [
                        HeaderRow
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBorder)
                    .BorderImage(WhiteBrush())
                    .BorderBackgroundColor(FSlateColor(Bg2()))
                    .Padding(FMargin(SpaceM, SpaceM))
                    [
                        InBody
                    ]
                ]
            ];
    }

    auto
    Make_KeyValueRow(
        const FText&      InKey,
        TAttribute<FText> InValue) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .Padding(FMargin(0.0f, SpaceXS))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, SpaceM, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(InKey)
                    .Font(Font_Regular(9))
                    .ColorAndOpacity(FSlateColor(TextMute()))
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(InValue)
                    .Font(Font_Bold(9))
                    .ColorAndOpacity(FSlateColor(Text()))
                    .AutoWrapText(true)
                ]
            ];
    }

    auto
    Make_CountBadge(
        int32 InCount) -> TSharedRef<SWidget>
    {
        return SNew(SBorder)
            .BorderImage(WhiteBrush())
            .BorderBackgroundColor(FSlateColor(Border()))
            .Padding(FMargin(1.0f))
            [
                SNew(SBorder)
                .BorderImage(WhiteBrush())
                .BorderBackgroundColor(FSlateColor(Bg3()))
                .Padding(FMargin(SpaceM, 1.0f))
                [
                    SNew(STextBlock)
                    .Text(FText::AsNumber(InCount))
                    .Font(Font_Bold(9))
                    .ColorAndOpacity(FSlateColor(TextDim()))
                ]
            ];
    }

    auto
    Make_Swatch(
        const FLinearColor& InColor,
        float               InSize) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .WidthOverride(InSize)
            .HeightOverride(InSize)
            [
                SNew(SImage)
                .Image(WhiteBrush())
                .ColorAndOpacity(FSlateColor(InColor))
            ];
    }
}

// --------------------------------------------------------------------------------------------------------------------
