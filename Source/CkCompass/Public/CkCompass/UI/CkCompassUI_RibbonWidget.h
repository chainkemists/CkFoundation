#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCompass/CkCompass_Fragment_Data.h"
#include "CkCompass/CkCompass_Utils.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCompassUI_RibbonWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCanvasPanel;
class UImage;
class UMaterialInstanceDynamic;
class UCk_CompassUI_MarkerWidget;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Consumer of the CkCompass delivery contract. The widget owns the MECHANISM (compass bind/unbind
 * lifecycle, heading/arc→material plumbing, arc math, marker pooling + positioning); the WBP owns
 * the TREE — author it and bind:
 *  - _RibbonImage → its brush material is wrapped in a MID receiving _HeadingParameterName /
 *    _ArcParameterName every frame.
 *  - _MarkerCanvas (+ set _MarkerWidgetClass) → one UCk_CompassUI_MarkerWidget per POI
 *    (DisplayDefinition-styled, distance readout), positioned by the widget.
 *  - _CardinalN.._CardinalNW → any styled widgets; the widget anchors them along the arc.
 *
 * Misconfigurations (markers requested without a _MarkerCanvas) are ensured at runtime and warned
 * at design time.
 *
 * Design-time: PreConstruct lays out _PreviewMarkerCount fake entries across _PreviewArcDegrees at
 * _PreviewHeadingDegrees through the SAME anchor math runtime uses, so the designer preview cannot
 * drift from runtime. Positioning is anchor-based (no geometry dependency).
 *
 * The Appeared/Disappeared signals are bound (IgnorePayloadInFlight) and surfaced as Blueprint events
 * to expose the membership-delta contract; positioning does NOT depend on them (index-pooled per frame).
 */
UCLASS(BlueprintType, Blueprintable)
class CKCOMPASS_API UCk_CompassUI_RibbonWidget : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CompassUI_RibbonWidget);

public:
    /** Bind this widget to a composed Compass feature — seeds current entries, then tracks
     *  membership deltas. Rebinding to a different compass unbinds the previous one. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Compass|Ribbon",
              DisplayName = "Inject Compass")
    void
    InjectCompass(
        UPARAM(ref) FCk_Handle_Compass& InCompass);

protected:
    /** Membership deltas, forwarded from the compass signals (bound with IgnorePayloadInFlight — initial
     *  population comes from Get_Entries inside InjectCompass, per the delivery contract). */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Compass|Ribbon")
    void
    OnCompassEntryAppeared(
        const FCk_Compass_Entry& InEntry);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Compass|Ribbon")
    void
    OnCompassEntryDisappeared(
        const FCk_Handle_Poi& InPoi);

protected:
    auto NativePreConstruct() -> void override;
    auto NativeDestruct() -> void override;
    auto NativeTick(const FGeometry& MyGeometry, float InDeltaTime) -> void override;

private:
    UFUNCTION()
    void
    HandleEntryAppeared(
        FCk_Handle_Compass InCompass,
        FCk_Compass_Entry InEntry);

    UFUNCTION()
    void
    HandleEntryDisappeared(
        FCk_Handle_Compass InCompass,
        FCk_Handle_Poi InPoi);

private:
    auto DoResolveBindings() -> void;
    auto DoLayoutPreview() -> void;
    auto DoRefreshLayout() -> void;
    auto DoPositionCardinals(float InHeading, float InArcDegrees) -> void;
    auto DoPositionChildAtOffset(UWidget* InChild, float InNormalizedOffset, float InBandNormalizedY) const -> void;
    auto Get_CardinalWidgets() const -> TArray<UWidget*, TInlineAllocator<8>>;
    auto Get_MarkerAt(int32 InIndex) -> UCk_CompassUI_MarkerWidget*;
    auto DoHideMarkersFromIndex(int32 InFirstHiddenIndex) -> void;
    auto DoReportMisconfig(const TCHAR* InMessage) -> void;

private:
    /** The ribbon strip. Its brush material is wrapped in a MID that receives
     *  _HeadingParameterName and _ArcParameterName every frame. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> _RibbonImage;

    /** The canvas the cardinal widgets + POI markers are anchored into. Required for markers/cardinals. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCanvasPanel> _MarkerCanvas;

    /** Cardinal direction widgets, anchored along the arc by the widget. Bind any styled widget;
     *  unbound directions simply don't render. All eight must live inside _MarkerCanvas. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalN;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalNE;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalE;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalSE;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalS;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalSW;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalW;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CardinalNW;

    /** Scalar parameter on the ribbon material that receives the heading (degrees, 0-360) every frame. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true))
    FName _HeadingParameterName = TEXT("Heading");

    /** Scalar parameter on the ribbon material that receives the compass arc / FOV (degrees) every
     *  frame — a material with baked tick/cardinal markings consumes this to size its visible
     *  window so the GPU strip and the widget-positioned markers can never drift. Writes to a
     *  material without this parameter are silent no-ops. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true))
    FName _ArcParameterName = TEXT("ArcDegrees");

    /** One styled widget per POI entry, pooled into _MarkerCanvas. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CompassUI_MarkerWidget> _MarkerWidgetClass;

    /** Clamped-to-edge entries render with this opacity multiplier so off-arc waypoints read as pinned. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _ClampedEntryOpacity = 0.5f;

    /** Normalized vertical band [0..1] the POI markers sit on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _MarkerBandY = 0.7f;

    /** Normalized vertical band [0..1] the cardinal widgets sit on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _CardinalBandY = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Preview",
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _PreviewMarkerCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Preview",
              meta = (AllowPrivateAccess = true))
    float _PreviewHeadingDegrees = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Preview",
              meta = (AllowPrivateAccess = true, ClampMin = "1.0", ClampMax = "360.0"))
    float _PreviewArcDegrees = 240.0f;

private:
    /** The compass this ribbon is bound to (set via InjectCompass). */
    UPROPERTY(Transient, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Compass _Compass;

    /** POIs whose membership-add signal has fired but that have not yet been laid out + notified.
     *  Drained in DoRefreshLayout so the "shown" pop plays on the exact marker that ends up showing the
     *  new POI, wherever it sorts this frame. Seed entries never route through HandleEntryAppeared, so
     *  binding a compass does NOT storm every already-in-range POI with a pop. */
    UPROPERTY(Transient)
    TArray<FCk_Handle_Poi> _PendingShownPois;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _RibbonMID;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_CompassUI_MarkerWidget>> _MarkerPool;

    bool _ReportedMisconfig = false;

public:
    CK_PROPERTY_GET(_Compass);
    CK_PROPERTY_GET(_HeadingParameterName);
    CK_PROPERTY_GET(_ArcParameterName);
    CK_PROPERTY_GET(_MarkerWidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------
