#include "CkSlateLayout/CkFlexText.h"

#include "Framework/Text/PlainTextLayoutMarshaller.h"
#include "Layout/Geometry.h"
#include "Widgets/Text/SlateTextBlockLayout.h"

namespace ck_slate_layout_flex_text
{
    bool IsFiniteNonNegative(const float InValue)
    {
        return FMath::IsFinite(InValue) && InValue >= 0.0f;
    }
}

SCkFlexText::SCkFlexText() = default;

SCkFlexText::~SCkFlexText() = default;

void SCkFlexText::Construct(const FArguments& InArgs)
{
    _Text = InArgs._Text;
    _Font = InArgs._Font;
    _ColorAndOpacity = InArgs._ColorAndOpacity;
    _WrapTextAt = InArgs._WrapTextAt;
    _Margin = InArgs._Margin;
    _LineHeightPercentage = InArgs._LineHeightPercentage;
    _Justification = InArgs._Justification;
    _WrappingPolicy = InArgs._WrappingPolicy;
    _AllowWrapping = InArgs._AllowWrapping;
    _OverflowPolicy = InArgs._OverflowPolicy;
    _TextStyle = InArgs._TextStyle != nullptr ? *InArgs._TextStyle : FTextBlockStyle::GetDefault();

    _TextLayout = MakeUnique<FSlateTextBlockLayout>(
        this,
        FTextBlockStyle::GetDefault(),
        InArgs._TextShapingMethod,
        InArgs._TextFlowDirection,
        FCreateSlateTextLayout(),
        FPlainTextLayoutMarshaller::Create(),
        InArgs._LineBreakPolicy);

    AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
        [WeakText = TWeakPtr<SCkFlexText>(SharedThis(this))](const FCkFlexMeasureArgs& InArgs) -> FVector2D
        {
            const TSharedPtr<SCkFlexText> Text = WeakText.Pin();
            return Text.IsValid()
                ? Text->Measure(InArgs.AvailableWidth, InArgs.WidthMode, InArgs.AvailableHeight, InArgs.HeightMode, InArgs.LayoutScale)
                : FVector2D::ZeroVector;
        },
        [WeakText = TWeakPtr<SCkFlexText>(SharedThis(this))](const float InWidth, const float)
        {
            if (const TSharedPtr<SCkFlexText> Text = WeakText.Pin())
            {
                Text->SetArrangedWidth(InWidth);
            }
        }));
}

FTextBlockStyle SCkFlexText::GetComputedTextStyle() const
{
    FTextBlockStyle Result = _TextStyle;
    const FSlateFontInfo& Font = _Font.Get();
    if (_Font.IsSet() && Font.Size > 0)
    {
        Result.SetFont(Font);
    }
    if (_ColorAndOpacity.IsSet())
    {
        Result.SetColorAndOpacity(_ColorAndOpacity.Get());
    }
    return Result;
}

void SCkFlexText::SetText(TAttribute<FText> InText)
{
    _Text = MoveTemp(InText);
    if (_TextLayout.IsValid())
    {
        _TextLayout->DirtyContent();
    }
    Invalidate(EInvalidateWidgetReason::Layout);
}

void SCkFlexText::SetFont(TAttribute<FSlateFontInfo> InFont)
{
    _Font = MoveTemp(InFont);
    if (_TextLayout.IsValid())
    {
        _TextLayout->DirtyLayout();
    }
    Invalidate(EInvalidateWidgetReason::Layout);
}

void SCkFlexText::SetColorAndOpacity(TAttribute<FSlateColor> InColorAndOpacity)
{
    _ColorAndOpacity = MoveTemp(InColorAndOpacity);
    if (_TextLayout.IsValid())
    {
        _TextLayout->DirtyLayout();
    }
    Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SCkFlexText::UpdateAndMeasure(const float InWrapWidth, const bool bHasExplicitWrapWidth, const float LayoutScaleMultiplier) const
{
    if (!_TextLayout.IsValid() || !FMath::IsFinite(LayoutScaleMultiplier) || LayoutScaleMultiplier <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }

    _TextLayout->ConditionallyUpdateTextStyle(GetComputedTextStyle());
    _TextLayout->SetTextOverflowPolicy(_OverflowPolicy.Get(ETextOverflowPolicy::Clip));

    // Yoga owns width constraints.  Passing false prevents Slate from consulting
    // its previous paint geometry, which is the source of one-frame-late wrapping.
    const FSlateTextBlockLayout::FWidgetDesiredSizeArgs LayoutArgs(
        _Text.Get(),
        FText::GetEmpty(),
        (_AllowWrapping.Get(true) && bHasExplicitWrapWidth) ? InWrapWidth : 0.0f,
        false,
        _WrappingPolicy.Get(),
        ETextTransformPolicy::None,
        _Margin.Get(),
        _LineHeightPercentage.Get(),
        _Justification.Get());

    return _TextLayout->ComputeDesiredSize(LayoutArgs, FMath::Max(LayoutScaleMultiplier, KINDA_SMALL_NUMBER));
}

FVector2D SCkFlexText::Measure(const float AvailableWidth, const YGMeasureMode WidthMode, const float AvailableHeight, const YGMeasureMode HeightMode, const float LayoutScaleMultiplier) const
{
    const auto MeasureArgs = FCkFlexMeasureArgs{
        .AvailableWidth = AvailableWidth,
        .WidthMode = WidthMode,
        .AvailableHeight = AvailableHeight,
        .HeightMode = HeightMode,
        .LayoutScale = LayoutScaleMultiplier,
    };
    if (!IsValid_CkFlexMeasureArgs(MeasureArgs))
    {
        return FVector2D::ZeroVector;
    }
    const bool bHasWidthConstraint = WidthMode != YGMeasureModeUndefined
        && ck_slate_layout_flex_text::IsFiniteNonNegative(AvailableWidth);
    const FVector2D NaturalSize = UpdateAndMeasure(AvailableWidth, bHasWidthConstraint, LayoutScaleMultiplier);

    FVector2D Result = NaturalSize;
    if (bHasWidthConstraint)
    {
        Result.X = WidthMode == YGMeasureModeExactly ? AvailableWidth : FMath::Min(Result.X, AvailableWidth);
    }

    if (HeightMode != YGMeasureModeUndefined && ck_slate_layout_flex_text::IsFiniteNonNegative(AvailableHeight))
    {
        Result.Y = HeightMode == YGMeasureModeExactly ? AvailableHeight : FMath::Min(Result.Y, AvailableHeight);
    }
    return Result;
}

void SCkFlexText::SetArrangedWidth(const float InArrangedWidth)
{
    const float SanitizedWidth = ck_slate_layout_flex_text::IsFiniteNonNegative(InArrangedWidth) ? InArrangedWidth : YGUndefined;
    if ((YGFloatIsUndefined(_ArrangedWidth) && YGFloatIsUndefined(SanitizedWidth))
        || (!YGFloatIsUndefined(_ArrangedWidth) && !YGFloatIsUndefined(SanitizedWidth) && FMath::IsNearlyEqual(_ArrangedWidth, SanitizedWidth)))
    {
        return;
    }

    _ArrangedWidth = SanitizedWidth;
    if (_TextLayout.IsValid())
    {
        _TextLayout->DirtyLayout();
    }
    Invalidate(EInvalidateWidgetReason::Layout);
}

int32 SCkFlexText::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    if (!_TextLayout.IsValid())
    {
        return LayerId;
    }

    const float Width = ck_slate_layout_flex_text::IsFiniteNonNegative(_ArrangedWidth)
        ? _ArrangedWidth
        : FMath::Max(0.0f, AllottedGeometry.GetLocalSize().X);
    if (!FMath::IsFinite(AllottedGeometry.Scale) || AllottedGeometry.Scale <= 0.0f)
    {
        return LayerId;
    }
    // Disabling soft wrapping retains explicit newlines, while the text layout's
    // overflow policy clips or ellipsizes during paint inside this finite geometry.
    UpdateAndMeasure(Width, _AllowWrapping.Get(true), AllottedGeometry.Scale);
    return _TextLayout->OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled(bParentEnabled));
}

FVector2D SCkFlexText::ComputeDesiredSize(const float LayoutScaleMultiplier) const
{
    return Measure(YGUndefined, YGMeasureModeUndefined, YGUndefined, YGMeasureModeUndefined, LayoutScaleMultiplier);
}
