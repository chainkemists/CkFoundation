#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkMinimap/CkMinimap_Fragment_Data.h"
#include "CkMinimap/CkMinimap_Utils.h"
#include "CkMinimap/CkMinimap_WorldBounds.h"

#include "CkUICore/UserWidget/CkUserWidget.h"

#include "CkMinimapFrame_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCanvasPanel;
class UImage;
class UTexture2D;
class UMaterialInstanceDynamic;

// --------------------------------------------------------------------------------------------------------------------

// Reference consumer of the CkMinimap delivery contract. The widget owns the MECHANISM (pull-in-tick
// projection consumption, blip pooling + positioning, background pan/zoom/rotate via UMG render
// transforms, frame-material yaw feed); the WBP owns the TREE — author it and bind:
//  - _BlipCanvas (required for blips/observer/background positioning; everything below lives in it)
//  - _MapImage → receives the game-supplied map texture (Set_MapTexture), panned/zoomed/rotated
//  - _FrameImage → its brush material is wrapped in a MID receiving _ViewYawParameterName
//  - _ObserverMarker → any styled widget, centered + yaw-rotated
// Blips are pooled instances of _BlipWidgetClass, positioned + rotated by the widget.
// Design notes (the pull-in-NativeTick shape, rectangle-only frames, fog, editor verification):
// CkMinimap/CLAUDE.md § "Reference widget".
UCLASS(Blueprintable, BlueprintType)
class CKMINIMAP_API UCk_MinimapFrame_Widget : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_MinimapFrame_Widget);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Minimap",
              DisplayName = "[Ck][Minimap] Set Minimap")
    void
    Set_Minimap(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|Minimap",
              DisplayName = "[Ck][Minimap] Get Minimap")
    FCk_Handle_Minimap
    Get_Minimap() const;

    // The background map imagery + the world rect it covers (typically both read off a
    // UCk_Minimap_MapLayer_PDA by the game). The widget never loads assets on its own.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Minimap",
              DisplayName = "[Ck][Minimap] Set Map Texture")
    void
    Set_MapTexture(
        UTexture2D* InTexture,
        const FCk_Minimap_WorldBounds& InBounds);

protected:
    // Membership deltas, forwarded from the minimap signals (bound with IgnorePayloadInFlight — initial
    // population comes from Get_Entries inside Set_Minimap, per the delivery contract)
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Minimap")
    void
    OnMinimapEntryAppeared(
        const FCk_Minimap_Entry& InEntry);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Minimap")
    void
    OnMinimapEntryDisappeared(
        const FCk_Handle_Poi& InPoi);

protected:
    auto NativeOnInitialized() -> void override;
    auto NativeDestruct() -> void override;
    auto NativeTick(const FGeometry& MyGeometry, float InDeltaTime) -> void override;

private:
    UFUNCTION()
    void
    HandleEntryAppeared(
        FCk_Handle_Minimap InMinimap,
        FCk_Minimap_Entry InEntry);

    UFUNCTION()
    void
    HandleEntryDisappeared(
        FCk_Handle_Minimap InMinimap,
        FCk_Handle_Poi InPoi);

private:
    auto DoRefreshLayout(const FGeometry& InGeometry) -> void;
    auto DoRefreshBackground(const FVector2D& InPanelCenter, float InFrameSize, float InViewYaw, bool InRotateWithObserver) -> void;
    auto Get_BlipAt(int32 InIndex) -> UUserWidget*;
    auto DoPositionChildAt(UWidget* InChild, const FVector2D& InPixelPosition) const -> void;

    // Screen-space rotation (UMG convention: +Y down, positive angle = clockwise — same sense as
    // UWidget::SetRenderTransformAngle)
    static auto DoRotate2D(const FVector2D& InVector, float InAngleDegrees) -> FVector2D;

private:
    /** The canvas everything is positioned in: _MapImage, _ObserverMarker, and the pooled blips. Required. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCanvasPanel> _BlipCanvas;

    /** Receives the game-supplied map texture; panned/zoomed/rotated by the widget. Must live in _BlipCanvas. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> _MapImage;

    /** Frame/mask image; its brush material is wrapped in a MID receiving _ViewYawParameterName every frame. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> _FrameImage;

    /** Any styled widget, centered in the frame and yaw-rotated. Must live in _BlipCanvas. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _ObserverMarker;

    /** One instance per POI entry, pooled into _BlipCanvas, positioned + yaw-rotated by the widget. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UUserWidget> _BlipWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true))
    FName _ViewYawParameterName = TEXT("ViewYaw");

    // Clamped-to-edge entries render with this opacity multiplier so off-frame waypoints read as pinned
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _ClampedBlipOpacity = 0.5f;

private:
    UPROPERTY(Transient)
    FCk_Handle_Minimap _Minimap;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> _MapTexture;

    UPROPERTY(Transient)
    FCk_Minimap_WorldBounds _MapBounds;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _FrameMID;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UUserWidget>> _BlipPool;

public:
    CK_PROPERTY_GET(_ViewYawParameterName);
    CK_PROPERTY_GET(_BlipWidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------
