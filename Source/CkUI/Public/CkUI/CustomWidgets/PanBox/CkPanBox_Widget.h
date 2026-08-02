#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkUI/CustomWidgets/PanBox/CkPanBox_Slate.h"
#include "CkUI/CustomWidgets/PanBox/CkPanBox_Types.h"

#include <Components/ContentWidget.h>
#include <CoreMinimal.h>

#include "CkPanBox_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_PanBox_ViewChangedEvent, const FCk_PanBox_View&, NewView);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Native-feeling pan-and-zoom container for a single child (the "surface"), clipped to
 * the box's own bounds (the "viewport").
 *
 * Input-agnostic and focus-agnostic: PanBox binds no mouse/touch/key/focus behavior of
 * its own. Drive it from your own input handling — capture an anchor with StartPan and
 * feed pointer deltas to Get_ViewForPan, zoom around the cursor with Get_ViewForZoomAt,
 * then apply the result with Request_SetView. Convert between spaces with Get_ViewportToSurface
 * and Get_SurfaceToViewport.
 *
 * All positions and deltas are in this widget's local Slate space ("viewport space");
 * ViewportPos = SurfacePos * Zoom + Offset. Request_SetView clamps zoom to the zoom range and
 * the offset per the bounds policy. Bounds clamping uses the viewport's last-known
 * geometry, so it is a no-op until the widget has been laid out once.
 */
UCLASS(meta = (DisplayName = "Ck Pan Box"))
class CKUI_API UCk_PanBoxWidget_UE : public UContentWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PanBoxWidget_UE);

public:
    UCk_PanBoxWidget_UE();

public:
    // Fired whenever the applied view actually changes (after clamping).
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|PanBox|Events")
    FCk_PanBox_ViewChangedEvent OnViewChanged;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|PanBox")
    void
    Request_SetView(
        const FCk_PanBox_View& InView);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FCk_PanBox_View
    Get_View() const;

    // Captures the current view as the pan anchor used by Get_ViewForPan, and returns it.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|PanBox")
    FCk_PanBox_View
    StartPan();

    // View that pans the anchor captured by StartPan by the given viewport-space delta, clamped.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FCk_PanBox_View
    Get_ViewForPan(
        FVector2D InViewportDelta) const;

    // View that zooms to InTargetZoom while keeping the surface point currently under
    // InViewportPoint stationary, clamped.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FCk_PanBox_View
    Get_ViewForZoomAt(
        FVector2D InViewportPoint,
        float InTargetZoom) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FVector2D
    Get_ViewportToSurface(
        FVector2D InViewportPos) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FVector2D
    Get_SurfaceToViewport(
        FVector2D InSurfacePos) const;

    // Resolved surface size: the explicit size, or the child's desired size.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FVector2D
    Get_SurfaceSize() const;

    // Size of the viewport (this widget's own last-known geometry).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|PanBox")
    FVector2D
    Get_ViewportSize() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|PanBox")
    void
    Request_SetZoomRange(
        const FCk_FloatRange& InZoomRange);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|PanBox")
    void
    Request_SetBoundsPolicy(
        ECk_PanBox_BoundsPolicy InBoundsPolicy);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|PanBox")
    void
    Request_SetSurfaceSizeMode(
        ECk_PanBox_SurfaceSizeMode InSurfaceSizeMode);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|PanBox")
    void
    Request_SetExplicitSurfaceSize(
        FVector2D InExplicitSurfaceSize);

public:
    auto SynchronizeProperties() -> void override;
    auto ReleaseSlateResources(bool bReleaseChildren) -> void override;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;
    auto OnSlotAdded(UPanelSlot* InSlot) -> void override;
    auto OnSlotRemoved(UPanelSlot* InSlot) -> void override;

private:
    UPROPERTY(EditAnywhere, Category = "PanBox",
              meta = (AllowPrivateAccess = true))
    FCk_PanBox_View _View;

    UPROPERTY(EditAnywhere, Category = "PanBox",
              meta = (AllowPrivateAccess = true))
    FCk_FloatRange _ZoomRange = FCk_FloatRange{0.1, 10.0};

    UPROPERTY(EditAnywhere, Category = "PanBox",
              meta = (AllowPrivateAccess = true))
    ECk_PanBox_BoundsPolicy _BoundsPolicy = ECk_PanBox_BoundsPolicy::ClampViewToSurface;

    UPROPERTY(EditAnywhere, Category = "PanBox",
              meta = (AllowPrivateAccess = true))
    ECk_PanBox_SurfaceSizeMode _SurfaceSizeMode = ECk_PanBox_SurfaceSizeMode::ChildDesiredSize;

    UPROPERTY(EditAnywhere, Category = "PanBox",
              meta = (AllowPrivateAccess = true, EditCondition = "_SurfaceSizeMode == ECk_PanBox_SurfaceSizeMode::Explicit"))
    FVector2D _ExplicitSurfaceSize = FVector2D{1024.0, 1024.0};

private:
    auto DoGet_ClampedView(const FCk_PanBox_View& InView) const -> FCk_PanBox_View;
    auto DoGet_ClampedZoom(float InZoom) const -> float;
    auto DoGet_ExplicitSurfaceSizeForSlate() const -> TOptional<FVector2D>;
    auto DoInvalidateLayout() -> void;

private:
    FCk_PanBox_View _PanAnchorView;

    TSharedPtr<SCk_PanBox> _PanBox;
};

// --------------------------------------------------------------------------------------------------------------------
