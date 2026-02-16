#include "CkWatermarkStat_Base_Widget.h"

#include "CkWatermark/Settings/CkWatermark_Settings.h"

#include <Widgets/Text/STextBlock.h>
#include <Widgets/SBoxPanel.h>

// --------------------------------------------------------------------------------------------------------------------

auto SCkWatermarkStat::Construct(const FArguments& InArgs) -> void
{
    // Font attributes (input) — evaluated each paint pass so outline settings are always live.
    const TAttribute<FSlateFontInfo> ValueFont_In   = InArgs._ValueFont;
    const TAttribute<FSlateFontInfo> LabelFont_In   = InArgs._LabelFont;
    const TAttribute<FSlateFontInfo> BracketFont_In = InArgs._BracketFont;
    const float InnerPad = InArgs._BracketInnerPadding;

    // Color attributes — evaluated each paint pass so settings changes are live.
    const TAttribute<FSlateColor> ValueColor = InArgs._ValueColor;
    const TAttribute<FSlateColor> LabelColor = InArgs._LabelColor;

    // Shadow — evaluated each paint pass so settings changes are live.
    const TAttribute<FVector2D>    ShadowOff   = InArgs._ShadowOffset;
    const TAttribute<FLinearColor> ShadowColor = InArgs._ShadowColorAndOpacity;

    // Wrap each font so its outline alpha tracks the paired text color's alpha.
    // The outline then fades consistently whenever the text color has alpha < 1.
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

    const TAttribute<FSlateFontInfo> ValueFont   = WithTextAlpha(ValueFont_In,   ValueColor);
    const TAttribute<FSlateFontInfo> LabelFont   = WithTextAlpha(LabelFont_In,   LabelColor);
    const TAttribute<FSlateFontInfo> BracketFont = WithTextAlpha(BracketFont_In, ValueColor);

    ChildSlot
    [
        SNew(SHorizontalBox)

        // Left bracket — size, type, and inner padding from project settings
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.f, 0.f, InnerPad, 0.f)
        [
            SNew(STextBlock)
            .Text(InArgs._BracketOpen)
            .Font(BracketFont)
            .ColorAndOpacity(InArgs._ValueColor)
            .ShadowOffset(ShadowOff)
            .ShadowColorAndOpacity(ShadowColor)
        ]

        // Value + name column
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SVerticalBox)

            // Stat value — colored, driven by TAttribute
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(InArgs._StatValue)
                .Font(ValueFont)
                .ColorAndOpacity(InArgs._ValueColor)
                .ShadowOffset(ShadowOff)
                .ShadowColorAndOpacity(ShadowColor)
            ]

            // Stat name — color and outline from project settings
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(InArgs._StatName)
                .Font(LabelFont)
                .ColorAndOpacity(LabelColor)
                .ShadowOffset(ShadowOff)
                .ShadowColorAndOpacity(ShadowColor)
            ]
        ]

        // Right bracket — size, type, and inner padding from project settings
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(InnerPad, 0.f, 0.f, 0.f)
        [
            SNew(STextBlock)
            .Text(InArgs._BracketClose)
            .Font(BracketFont)
            .ColorAndOpacity(InArgs._ValueColor)
            .ShadowOffset(ShadowOff)
            .ShadowColorAndOpacity(ShadowColor)
        ]
    ];
}

// --------------------------------------------------------------------------------------------------------------------

UCkWatermarkStat_Base_UWidget_UE::UCkWatermarkStat_Base_UWidget_UE()
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_Base_UWidget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    // Build font attributes — lambdas re-read outline settings each paint pass.
    auto MakeFont = [](const char* InFace, int32 InSize) -> TAttribute<FSlateFontInfo>
    {
        return TAttribute<FSlateFontInfo>::CreateLambda([InFace, InSize]() -> FSlateFontInfo
        {
            FSlateFontInfo F = FCoreStyle::GetDefaultFontStyle(InFace, InSize);
            F.OutlineSettings.OutlineSize  = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextOutlineSize();
            F.OutlineSettings.OutlineColor = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextOutlineColor();
            return F;
        });
    };

    const TAttribute<FSlateFontInfo> ValueFont   = MakeFont("Bold",    UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_ValueFontSize());
    const TAttribute<FSlateFontInfo> LabelFont   = MakeFont("Regular", UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_LabelFontSize());
    const TAttribute<FSlateFontInfo> BracketFont = MakeFont("Bold",    UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BracketFontSize());

    const FText BracketOpen  = FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BracketOpen());
    const FText BracketClose = FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BracketClose());

    const float InnerPad = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Bracket_InnerPadding();
    const TAttribute<FSlateColor> LabelColor = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_LabelColor());
    });

    const TAttribute<FVector2D> ShadowOffsetAttr = TAttribute<FVector2D>::CreateLambda([]() -> FVector2D
    {
        return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextShadowOffset();
    });

    const TAttribute<FLinearColor> ShadowColorAttr = TAttribute<FLinearColor>::CreateLambda([]() -> FLinearColor
    {
        return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextShadowColor();
    });

    // TAttribute lambdas — evaluated each Slate paint pass, no tick required
    const TAttribute<FText> ValueAttr = TAttribute<FText>::CreateWeakLambda(
        this,
        [this]() -> FText
        {
            return NativeGetStatValue();
        });

    const TAttribute<FSlateColor> ColorAttr = TAttribute<FSlateColor>::CreateWeakLambda(
        this,
        [this]() -> FSlateColor
        {
            return FSlateColor(NativeGetStatColor());
        });

    _SlateWidget = SNew(SCkWatermarkStat)
        .StatName(NativeGetStatName())
        .ValueFont(ValueFont)
        .LabelFont(LabelFont)
        .BracketFont(BracketFont)
        .BracketOpen(BracketOpen)
        .BracketClose(BracketClose)
        .BracketInnerPadding(InnerPad)
        .LabelColor(LabelColor)
        .ShadowOffset(ShadowOffsetAttr)
        .ShadowColorAndOpacity(ShadowColorAttr)
        .StatValue(ValueAttr)
        .ValueColor(ColorAttr);

    return _SlateWidget.ToSharedRef();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_Base_UWidget_UE::
    ReleaseSlateResources(bool bReleaseChildren)
    -> void
{
    Super::ReleaseSlateResources(bReleaseChildren);
    _SlateWidget.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_Base_UWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    // Font or name changes require a full Slate widget rebuild
    if (_SlateWidget.IsValid())
    {
        TakeWidget();
    }
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCkWatermarkStat_Base_UWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return FText::FromString(TEXT("CkFoundation|Watermark Stats"));
}
#endif

// --------------------------------------------------------------------------------------------------------------------
