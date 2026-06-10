#include "CkColorWheel_Widget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Styling/CoreStyle.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_ColorWheelWidget_UE::
    UCk_ColorWheelWidget_UE()
{
    if (IsRunningDedicatedServer())
    { return; }

    _HueCircleBrush = *FCoreStyle::Get().GetBrush(TEXT("ColorWheel.HueValueCircle"));
    _SelectorPinBrush = *FCoreStyle::Get().GetBrush(TEXT("ColorWheel.Selector"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ColorWheelWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    DoUpdateColor(_Color);
    DoInvalidateWheelPaint();
}

auto
    UCk_ColorWheelWidget_UE::
    ReleaseSlateResources(
        bool bReleaseChildren)
    -> void
{
    Super::ReleaseSlateResources(bReleaseChildren);

    _ColorWheel.Reset();
}

#if WITH_EDITOR
auto
    UCk_ColorWheelWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_ColorWheelWidget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    _ColorWheel = SNew(SCk_ColorWheel)
        .HueCircleBrush(&_HueCircleBrush)
        .SelectorPinBrush(&_SelectorPinBrush)
        .SelectedColorHSV_UObject(this, &UCk_ColorWheelWidget_UE::Get_ColorHSV)
        .OnDragStarted(FSimpleDelegate::CreateUObject(this, &UCk_ColorWheelWidget_UE::HandleDragStarted))
        .OnDragFinished(FSimpleDelegate::CreateUObject(this, &UCk_ColorWheelWidget_UE::HandleDragFinished))
        .OnValueChanged(FOnLinearColorValueChanged::CreateUObject(this, &UCk_ColorWheelWidget_UE::HandleValueChanged))
        .OnPositionChanged(FCk_Delegate_ColorWheel_PositionChanged::CreateUObject(this, &UCk_ColorWheelWidget_UE::HandlePositionChanged));

    return _ColorWheel.ToSharedRef();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ColorWheelWidget_UE::
    Set_Color(
        FLinearColor InColor)
    -> void
{
    DoUpdateColor(InColor);
    DoInvalidateWheelPaint();
}

auto
    UCk_ColorWheelWidget_UE::
    Get_Color() const
    -> FLinearColor
{
    return _Color;
}

auto
    UCk_ColorWheelWidget_UE::
    Set_BrushTint(
        ECk_ColorWheel_BrushTarget InTarget,
        FLinearColor InTint)
    -> void
{
    const auto TintColor = FSlateColor{InTint};

    switch (InTarget)
    {
        case ECk_ColorWheel_BrushTarget::All:
        {
            _HueCircleBrush.TintColor = TintColor;
            _SelectorPinBrush.TintColor = TintColor;
            break;
        }
        case ECk_ColorWheel_BrushTarget::SelectorPin:
        {
            _SelectorPinBrush.TintColor = TintColor;
            break;
        }
        case ECk_ColorWheel_BrushTarget::HueCircle:
        {
            _HueCircleBrush.TintColor = TintColor;
            break;
        }
    }

    DoInvalidateWheelPaint();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ColorWheelWidget_UE::
    DoUpdateColor(
        const FLinearColor& InColor)
    -> void
{
    auto NewColorHSV = InColor.LinearRGBToHSV();

    // Black carries no hue/saturation and greys/white no hue — keep the pin where it
    // was instead of snapping it to the wheel center (the source project worked around
    // this by substituting white and flagging the color as "null").
    if (FMath::IsNearlyZero(NewColorHSV.B))
    {
        NewColorHSV.R = _ColorHSV.R;
        NewColorHSV.G = _ColorHSV.G;
    }
    else if (FMath::IsNearlyZero(NewColorHSV.G))
    {
        NewColorHSV.R = _ColorHSV.R;
    }

    _ColorHSV = NewColorHSV;
    _Color = InColor;
}

auto
    UCk_ColorWheelWidget_UE::
    DoInvalidateWheelPaint()
    -> void
{
    if (ck::Is_NOT_Valid(_ColorWheel))
    { return; }

    _ColorWheel->Invalidate(EInvalidateWidgetReason::Paint);
}

auto
    UCk_ColorWheelWidget_UE::
    Get_ColorHSV() const
    -> FLinearColor
{
    return _ColorHSV;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ColorWheelWidget_UE::
    HandleValueChanged(
        FLinearColor InNewColorHSV)
    -> void
{
    _ColorHSV = InNewColorHSV;
    _Color = InNewColorHSV.HSVToLinearRGB();

    OnColorChanged.Broadcast(_Color);
}

auto
    UCk_ColorWheelWidget_UE::
    HandlePositionChanged(
        FVector2D InNewPosition)
    -> void
{
    OnPositionChanged.Broadcast(InNewPosition);
}

auto
    UCk_ColorWheelWidget_UE::
    HandleDragStarted()
    -> void
{
    OnDragStarted.Broadcast();
}

auto
    UCk_ColorWheelWidget_UE::
    HandleDragFinished()
    -> void
{
    OnDragFinished.Broadcast();
}

// --------------------------------------------------------------------------------------------------------------------
