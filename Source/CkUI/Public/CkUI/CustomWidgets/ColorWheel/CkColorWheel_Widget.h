#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkUI/CustomWidgets/ColorWheel/CkColorWheel_Slate.h"

#include <CoreMinimal.h>
#include <Components/Widget.h>
#include <Styling/SlateBrush.h>

#include "CkColorWheel_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCk_ColorWheel_Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_ColorWheel_ColorChangedEvent, const FLinearColor&, NewColor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_ColorWheel_PositionChangedEvent, const FVector2D&, NewPosition);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ColorWheel_BrushTarget : uint8
{
    All,
    SelectorPin,
    HueCircle
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ColorWheel_BrushTarget);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Hue/saturation color wheel with a draggable selector pin.
 * Configure the brushes and initial color below; usable directly with no WBP wrapper.
 *
 * The wheel edits hue (angle) and saturation (radius); the color's value (brightness)
 * and alpha are preserved from whatever was last passed to Set_Color.
 */
UCLASS(meta = (DisplayName = "Ck Color Wheel"))
class CKUI_API UCk_ColorWheelWidget_UE : public UWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ColorWheelWidget_UE);

public:
    UCk_ColorWheelWidget_UE();

public:
    // Fired with the new RGB color whenever the selection changes (continuously while dragging).
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|ColorWheel|Events")
    FCk_ColorWheel_ColorChangedEvent OnColorChanged;

    // Fired with the selector position relative to the wheel center, X/Y in [-1..1], Y up.
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|ColorWheel|Events")
    FCk_ColorWheel_PositionChangedEvent OnPositionChanged;

    // Fired when the wheel is pressed and a drag begins.
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|ColorWheel|Events")
    FCk_ColorWheel_Event OnDragStarted;

    // Fired when the drag ends.
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|ColorWheel|Events")
    FCk_ColorWheel_Event OnDragFinished;

public:
    // Sets the selected color (RGB). Achromatic colors (black/white/greys) keep the
    // pin's current hue/saturation since they don't define one themselves.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|ColorWheel")
    void
    Set_Color(
        FLinearColor InColor);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|ColorWheel")
    FLinearColor
    Get_Color() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|ColorWheel")
    void
    Set_BrushTint(
        ECk_ColorWheel_BrushTarget InTarget,
        FLinearColor InTint);

public:
    auto SynchronizeProperties() -> void override;
    auto ReleaseSlateResources(bool bReleaseChildren) -> void override;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

protected:
    UPROPERTY(EditAnywhere, Category = "ColorWheel",
              meta = (AllowPrivateAccess = true))
    FSlateBrush _HueCircleBrush;

    UPROPERTY(EditAnywhere, Category = "ColorWheel",
              meta = (AllowPrivateAccess = true))
    FSlateBrush _SelectorPinBrush;

    UPROPERTY(EditAnywhere, Category = "ColorWheel",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

private:
    // Applies an RGB color to the internal HSV state, preserving hue/saturation for
    // achromatic inputs so the selector pin doesn't snap to the center.
    auto DoUpdateColor(const FLinearColor& InColor) -> void;
    auto DoInvalidateWheelPaint() -> void;

    auto Get_ColorHSV() const -> FLinearColor;

    auto HandleValueChanged(FLinearColor InNewColorHSV) -> void;
    auto HandlePositionChanged(FVector2D InNewPosition) -> void;
    auto HandleDragStarted() -> void;
    auto HandleDragFinished() -> void;

private:
    // The selected color in HSV space: R = hue degrees [0..360), G = saturation,
    // B = value, A = alpha. Source of truth for the Slate widget's attribute.
    FLinearColor _ColorHSV = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);

    TSharedPtr<SCk_ColorWheel> _ColorWheel;
};

// --------------------------------------------------------------------------------------------------------------------
