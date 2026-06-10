#include "CkColorWheel_Slate.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Rendering/DrawElements.h>
#include <Styling/CoreStyle.h>
#include <Styling/SlateBrush.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_ColorWheel::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _HueCircleBrush = ck::IsValid(InArgs._HueCircleBrush, ck::IsValid_Policy_NullptrOnly{})
        ? InArgs._HueCircleBrush
        : FCoreStyle::Get().GetBrush(TEXT("ColorWheel.HueValueCircle"));

    _SelectorPinBrush = ck::IsValid(InArgs._SelectorPinBrush, ck::IsValid_Policy_NullptrOnly{})
        ? InArgs._SelectorPinBrush
        : FCoreStyle::Get().GetBrush(TEXT("ColorWheel.Selector"));

    _SelectedColorHSV = InArgs._SelectedColorHSV;
    _OnDragStarted = InArgs._OnDragStarted;
    _OnDragFinished = InArgs._OnDragFinished;
    _OnValueChanged = InArgs._OnValueChanged;
    _OnPositionChanged = InArgs._OnPositionChanged;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_ColorWheel::
    ComputeDesiredSize(float) const
    -> FVector2D
{
    return FVector2D{FVector2f{_HueCircleBrush->ImageSize} + FVector2f{_SelectorPinBrush->ImageSize}};
}

auto
    SCk_ColorWheel::
    OnMouseButtonDown(
        const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent)
    -> FReply
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    { return FReply::Unhandled(); }

    _OnDragStarted.ExecuteIfBound();

    constexpr auto AllowOutsideWheel = false;
    if (NOT ProcessMouseAction(MyGeometry, MouseEvent, AllowOutsideWheel))
    {
        _OnDragFinished.ExecuteIfBound();
        return FReply::Unhandled();
    }

    return FReply::Handled().CaptureMouse(SharedThis(this));
}

auto
    SCk_ColorWheel::
    OnMouseButtonUp(
        const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent)
    -> FReply
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || NOT HasMouseCapture())
    { return FReply::Unhandled(); }

    _OnDragFinished.ExecuteIfBound();

    return FReply::Handled().ReleaseMouseCapture();
}

auto
    SCk_ColorWheel::
    OnMouseMove(
        const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent)
    -> FReply
{
    if (NOT HasMouseCapture())
    { return FReply::Unhandled(); }

    // While dragging, keep tracking even past the rim — the radius clamps to the wheel edge.
    constexpr auto AllowOutsideWheel = true;
    ProcessMouseAction(MyGeometry, MouseEvent, AllowOutsideWheel);

    return FReply::Handled();
}

auto
    SCk_ColorWheel::
    OnMouseButtonDoubleClick(
        const FGeometry& MyGeometry,
        const FPointerEvent& MouseEvent)
    -> FReply
{
    // Swallow double-clicks so they don't bubble to widgets behind the wheel.
    return FReply::Handled();
}

auto
    SCk_ColorWheel::
    OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const
    -> int32
{
    const auto IsWheelEnabled = ShouldBeEnabled(bParentEnabled);
    const auto DrawEffects = IsWheelEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

    const auto LocalSize = FVector2f{AllottedGeometry.GetLocalSize()};
    const auto SelectorSize = FVector2f{_SelectorPinBrush->ImageSize};

    // The wheel is inset by half the selector so the pin stays fully visible at the rim.
    const auto WheelSize = LocalSize - SelectorSize;

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(WheelSize, FSlateLayoutTransform{0.5f * SelectorSize}),
        _HueCircleBrush,
        DrawEffects,
        InWidgetStyle.GetColorAndOpacityTint() * _HueCircleBrush->GetTint(InWidgetStyle));

    const auto SelectorTopLeft =
        0.5f * (LocalSize + FVector2f{Get_RelativePositionFromCenter()} * WheelSize - SelectorSize);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(SelectorSize, FSlateLayoutTransform{SelectorTopLeft}),
        _SelectorPinBrush,
        DrawEffects,
        InWidgetStyle.GetColorAndOpacityTint() * _SelectorPinBrush->GetTint(InWidgetStyle));

    return LayerId + 1;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_ColorWheel::
    Get_RelativePositionFromCenter() const
    -> FVector2D
{
    const auto& ColorHSV = _SelectedColorHSV.Get();

    const auto HueAngleRadians = ColorHSV.R / 180.0f * PI;
    const auto Saturation = ColorHSV.G;

    return FVector2D{FMath::Cos(HueAngleRadians), FMath::Sin(HueAngleRadians)} * Saturation;
}

auto
    SCk_ColorWheel::
    ProcessMouseAction(
        const FGeometry& InMyGeometry,
        const FPointerEvent& InMouseEvent,
        bool InAllowOutsideWheel)
    -> bool
{
    const auto LocalMouseCoordinate = FVector2f{InMyGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())};
    const auto LocalSize = FVector2f{InMyGeometry.GetLocalSize()};
    const auto SelectorSize = FVector2f{_SelectorPinBrush->ImageSize};

    const auto RelativePositionFromCenter = (2.0f * LocalMouseCoordinate - LocalSize) / (LocalSize - SelectorSize);
    const auto RelativeRadius = RelativePositionFromCenter.Size();

    // Reported with Y up (screen-space Y points down) so consumers get conventional math coordinates.
    _OnPositionChanged.ExecuteIfBound(FVector2D{
        FMath::Clamp(RelativePositionFromCenter.X, -1.0f, 1.0f),
        -FMath::Clamp(RelativePositionFromCenter.Y, -1.0f, 1.0f)});

    const auto IsInsideWheel = RelativeRadius <= 1.0f;

    if (IsInsideWheel || InAllowOutsideWheel)
    {
        auto HueAngleRadians = FMath::Atan2(RelativePositionFromCenter.Y, RelativePositionFromCenter.X);

        if (HueAngleRadians < 0.0f)
        { HueAngleRadians += 2.0f * PI; }

        auto NewColorHSV = _SelectedColorHSV.Get();
        NewColorHSV.R = HueAngleRadians * 180.0f * INV_PI;
        NewColorHSV.G = FMath::Min(RelativeRadius, 1.0f);

        _OnValueChanged.ExecuteIfBound(NewColorHSV);
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    return IsInsideWheel;
}

// --------------------------------------------------------------------------------------------------------------------
