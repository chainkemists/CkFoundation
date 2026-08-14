#pragma once

#include <CoreMinimal.h>
#include <Framework/SlateDelegates.h>
#include <Input/Reply.h>
#include <Misc/Attribute.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SLeafWidget.h>

// --------------------------------------------------------------------------------------------------------------------

class FPaintArgs;
class FSlateWindowElementList;
struct FSlateBrush;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_OneParam(FCk_Delegate_ColorWheel_PositionChanged, FVector2D);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Hue/saturation color wheel with a draggable selector pin.
 *
 * Like the engine's SColorWheel, except the hue-circle and selector-pin brushes are
 * supplied as arguments instead of being hardcoded style lookups.
 *
 * The SelectedColorHSV attribute (and the color passed to OnValueChanged) is in HSV space:
 * R = hue in degrees [0..360), G = saturation [0..1]; B (value) and A (alpha) pass through
 * untouched. The UMG wrapper (UCk_ColorWheelWidget_UE) handles the RGB <-> HSV conversion.
 */
class CKWIDGETS_API SCk_ColorWheel : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SCk_ColorWheel)
        : _HueCircleBrush(nullptr)
        , _SelectorPinBrush(nullptr)
        , _SelectedColorHSV(FLinearColor(0.0f, 0.0f, 1.0f, 1.0f))
    {}
        // Brush drawn as the wheel itself. Falls back to the core-style hue circle when unset.
        SLATE_ARGUMENT(const FSlateBrush*, HueCircleBrush)

        // Brush drawn as the draggable selector pin. Falls back to the core-style selector when unset.
        SLATE_ARGUMENT(const FSlateBrush*, SelectorPinBrush)

        SLATE_ATTRIBUTE(FLinearColor, SelectedColorHSV)

        SLATE_EVENT(FSimpleDelegate, OnDragStarted)

        SLATE_EVENT(FSimpleDelegate, OnDragFinished)

        SLATE_EVENT(FOnLinearColorValueChanged, OnValueChanged)

        // Invoked with the selector position relative to the wheel center, X/Y in [-1..1], Y up.
        SLATE_EVENT(FCk_Delegate_ColorWheel_PositionChanged, OnPositionChanged)
    SLATE_END_ARGS()

public:
    auto Construct(const FArguments& InArgs) -> void;

public:
    auto ComputeDesiredSize(float) const -> FVector2D override;
    auto OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    auto OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    auto OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    auto OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;

    auto OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const -> int32 override;

private:
    auto Get_RelativePositionFromCenter() const -> FVector2D;

    // Returns whether the cursor was inside the wheel.
    auto ProcessMouseAction(
        const FGeometry& InMyGeometry,
        const FPointerEvent& InMouseEvent,
        bool InAllowOutsideWheel) -> bool;

private:
    const FSlateBrush* _HueCircleBrush = nullptr;
    const FSlateBrush* _SelectorPinBrush = nullptr;
    TAttribute<FLinearColor> _SelectedColorHSV;

    FSimpleDelegate _OnDragStarted;
    FSimpleDelegate _OnDragFinished;
    FOnLinearColorValueChanged _OnValueChanged;
    FCk_Delegate_ColorWheel_PositionChanged _OnPositionChanged;
};

// --------------------------------------------------------------------------------------------------------------------
