#pragma once

#include "CoreMinimal.h"
#include "CkFlexLayoutTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/SLeafWidget.h"

class FSlateTextBlockLayout;
class IBreakIterator;
enum class ETextShapingMethod : uint8;

/**
 * A text leaf whose wrapping width is supplied by SCkFlexBox during Yoga layout.
 * It never uses Slate's geometry-derived automatic wrapping.
 */
class CKSLATELAYOUT_API SCkFlexText final : public SLeafWidget
{
public:
    SCkFlexText();

    SLATE_BEGIN_ARGS(SCkFlexText)
        : _Text()
        , _TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
        , _Font()
        , _ColorAndOpacity()
        , _WrapTextAt(0.0f)
        , _Margin()
        , _LineHeightPercentage(1.0f)
        , _Justification(ETextJustify::Left)
        , _WrappingPolicy(ETextWrappingPolicy::DefaultWrapping)
        , _AllowWrapping(true)
        , _OverflowPolicy(ETextOverflowPolicy::Clip)
        , _TextShapingMethod()
        , _TextFlowDirection()
        , _LineBreakPolicy()
    {
    }
        SLATE_ATTRIBUTE(FText, Text)
        SLATE_ARGUMENT(const FTextBlockStyle*, TextStyle)
        SLATE_ATTRIBUTE(FSlateFontInfo, Font)
        SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)
        SLATE_ATTRIBUTE(float, WrapTextAt)
        SLATE_ATTRIBUTE(FMargin, Margin)
        SLATE_ATTRIBUTE(float, LineHeightPercentage)
        SLATE_ATTRIBUTE(ETextJustify::Type, Justification)
        SLATE_ATTRIBUTE(ETextWrappingPolicy, WrappingPolicy)
        SLATE_ATTRIBUTE(bool, AllowWrapping)
        SLATE_ATTRIBUTE(ETextOverflowPolicy, OverflowPolicy)
        SLATE_ARGUMENT(TOptional<ETextShapingMethod>, TextShapingMethod)
        SLATE_ARGUMENT(TOptional<ETextFlowDirection>, TextFlowDirection)
        SLATE_ARGUMENT(TSharedPtr<IBreakIterator>, LineBreakPolicy)
    SLATE_END_ARGS()

    ~SCkFlexText();

    void Construct(const FArguments& InArgs);

    FVector2D Measure(float AvailableWidth, YGMeasureMode WidthMode, float AvailableHeight, YGMeasureMode HeightMode, float LayoutScaleMultiplier) const;
    void SetArrangedWidth(float InArrangedWidth);
    void SetText(TAttribute<FText> InText);
    FText GetText() const { return _Text.Get(FText::GetEmpty()); }
    FSlateColor GetColorAndOpacity() const { return _ColorAndOpacity.Get(FSlateColor::UseForeground()); }
    void SetFont(TAttribute<FSlateFontInfo> InFont);
    void SetColorAndOpacity(TAttribute<FSlateColor> InColorAndOpacity);

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
    FVector2D UpdateAndMeasure(float InWrapWidth, bool bHasExplicitWrapWidth, float LayoutScaleMultiplier) const;
    FTextBlockStyle GetComputedTextStyle() const;

    TAttribute<FText> _Text;
    TAttribute<FSlateFontInfo> _Font;
    TAttribute<FSlateColor> _ColorAndOpacity;
    TAttribute<float> _WrapTextAt;
    TAttribute<FMargin> _Margin;
    TAttribute<float> _LineHeightPercentage;
    TAttribute<ETextJustify::Type> _Justification;
    TAttribute<ETextWrappingPolicy> _WrappingPolicy;
    TAttribute<bool> _AllowWrapping;
    TAttribute<ETextOverflowPolicy> _OverflowPolicy;
    FTextBlockStyle _TextStyle;
    mutable TUniquePtr<FSlateTextBlockLayout> _TextLayout;
    float _ArrangedWidth = YGUndefined;
};
