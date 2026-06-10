// Inspired by OmegaGameFramework's SWColorWheel (itself derived from the engine's SColorWheel):
// https://github.com/StudioSyndiCatCaius/OmegaGameFramework
//
// Improvements over the source (per CkFoundation conventions): engine-correct selector
// placement math (the source dropped the half-selector offset, leaving the pin off-center),
// removal of dead tint attributes (tinting goes through the brushes the UMG wrapper owns),
// self-invalidation on value changes, and ck::IsValid guards with core-style brush fallbacks.

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
class CKUI_API SCk_ColorWheel : public SLeafWidget
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

        // The currently selected color, in HSV space (see class comment).
        SLATE_ATTRIBUTE(FLinearColor, SelectedColorHSV)

        // Invoked when the left mouse button is pressed inside the wheel and a drag begins.
        SLATE_EVENT(FSimpleDelegate, OnDragStarted)

        // Invoked when the mouse is released and the drag ends.
        SLATE_EVENT(FSimpleDelegate, OnDragFinished)

        // Invoked with the new HSV color whenever the selection changes.
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
    // Selector position relative to the wheel center, derived from hue (angle) and saturation (radius).
    auto Get_RelativePositionFromCenter() const -> FVector2D;

    // Updates the selection from a mouse press/move. Returns whether the cursor was inside the wheel.
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
