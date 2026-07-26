#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkMinimap/CkMinimap_Fragment_Data.h"
#include "CkMinimap/CkMinimap_Utils.h"
#include "CkMinimap/CkMinimap_WorldBounds.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkMinimapFrame_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCanvasPanel;
class UImage;
class UTextBlock;
class UTexture2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

// --------------------------------------------------------------------------------------------------------------------

// Reference consumer of the CkMinimap delivery contract, fully self-contained (ZERO asset dependencies):
// POI blips are pooled Images positioned in ONE loop per frame from the pull API (Get_Entries), the observer
// marker is a text glyph, and the optional map background is a game-supplied texture (Set_MapTexture) panned/
// zoomed/rotated purely through UMG render transforms — no material required. An optional frame material
// receives the view yaw as a scalar parameter for GPU-drawn frames/masks.
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
    auto DoBuildWidgetTree() -> void;
    auto DoRefreshLayout(const FGeometry& InGeometry) -> void;
    auto DoRefreshBackground(const FVector2D& InPanelCenter, float InFrameSize, float InViewYaw, bool InRotateWithObserver) -> void;
    auto DoPositionChildAt(UWidget* InChild, const FVector2D& InPixelPosition) const -> void;

    // Screen-space rotation (UMG convention: +Y down, positive angle = clockwise — same sense as
    // UWidget::SetRenderTransformAngle)
    static auto DoRotate2D(const FVector2D& InVector, float InAngleDegrees) -> FVector2D;

private:
    // Optional — a frame/mask material; receives the view yaw (degrees, 0-360) on _ViewYawParameterName
    // every frame via a dynamic instance. Leave unset for the pure-widget presentation.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _FrameMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true))
    FName _ViewYawParameterName = TEXT("ViewYaw");

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true))
    FVector2D _BlipSize = FVector2D{12.0, 12.0};

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Minimap",
              meta = (AllowPrivateAccess = true))
    FLinearColor _BlipTint = FLinearColor::White;

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
    TObjectPtr<UCanvasPanel> _RootCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UImage> _MapImage;

    UPROPERTY(Transient)
    TObjectPtr<UImage> _FrameImage;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _FrameMID;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> _ObserverArrow;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UImage>> _BlipPool;

public:
    CK_PROPERTY_GET(_FrameMaterial);
    CK_PROPERTY_GET(_ViewYawParameterName);
};

// --------------------------------------------------------------------------------------------------------------------
