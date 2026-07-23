#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCompass/CkCompass_Fragment_Data.h"
#include "CkCompass/CkCompass_Utils.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCompassUI_RibbonWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCanvasPanel;
class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UCk_CompassUI_MarkerWidget;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CompassRibbon_Presentation : uint8
{
    // Zero-asset debug/default presentation: the widget code-builds its whole tree (root canvas,
    // cardinal letters, debug icon markers, optional _RibbonMaterial strip). The WBP must NOT
    // author a widget tree.
    CodeBuilt,

    // Designer-authored: the WBP owns the tree. Bind _RibbonImage to have its brush material fed
    // the heading/arc scalars; bind _MarkerCanvas (+ set _MarkerWidgetClass) for POI markers.
    // Nothing is code-built at the root — a material with baked cardinal markings only needs
    // _RibbonImage bound and _ShowCardinals off.
    Custom
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CompassRibbon_Presentation);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Consumer of the CkCompass delivery contract. The widget owns the MECHANISM (compass bind/unbind
 * lifecycle, heading/arc→material plumbing, arc math, marker pooling + positioning); _Presentation
 * declares who owns the TREE — ZERO graph/code either way:
 *  - CodeBuilt: empty WBP; the widget builds the debug look (canvas, cardinal letters, debug icon
 *               markers, optional _RibbonMaterial strip).
 *  - Custom:    the WBP authors the tree. Bind _RibbonImage → its brush material is wrapped in a MID
 *               receiving _HeadingParameterName/_ArcParameterName every frame. Bind _MarkerCanvas
 *               (+ set _MarkerWidgetClass) → one UCk_CompassUI_MarkerWidget per POI
 *               (DisplayDefinition-styled, distance readout), positioned by the widget.
 *
 * Misconfigurations (CodeBuilt with an authored tree; Custom markers without _MarkerCanvas) are
 * ensured at runtime and warned at design time.
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
    auto DoResolvePresentation() -> void;
    auto DoLayoutPreview() -> void;
    auto DoRefreshLayout() -> void;
    auto DoPositionCardinals(float InHeading, float InArcDegrees) -> void;
    auto DoPositionChildAtOffset(UWidget* InChild, float InNormalizedOffset, float InBandNormalizedY) const -> void;
    auto Get_EffectiveCanvas() const -> UCanvasPanel*;
    auto Get_MarkerAt(int32 InIndex) -> UWidget*;
    auto DoHideMarkersFromIndex(int32 InFirstHiddenIndex) -> void;
    auto DoReportMisconfig(const TCHAR* InMessage) -> void;

private:
    /** Custom mode: the designer-authored ribbon strip. Name a UImage "_RibbonImage" in the WBP;
     *  its brush material is wrapped in a MID that receives _HeadingParameterName and
     *  _ArcParameterName every frame. (CodeBuilt mode uses _RibbonMaterial instead.) */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> _RibbonImage;

    /** Custom mode: the canvas the cardinal labels + POI markers are injected into. Name a
     *  UCanvasPanel "_MarkerCanvas" in the WBP — required there for markers/cardinals; in
     *  CodeBuilt mode the code-built root canvas is used instead. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCanvasPanel> _MarkerCanvas;

    /** Who owns the widget tree — see class comment. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true))
    ECk_CompassRibbon_Presentation _Presentation = ECk_CompassRibbon_Presentation::CodeBuilt;

    /** Ribbon material for the code-built strip (CodeBuilt only — in Custom, author the material
     *  on the bound _RibbonImage's brush instead). Leave unset for the pure-widget presentation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true, EditCondition = "_Presentation == ECk_CompassRibbon_Presentation::CodeBuilt"))
    TObjectPtr<UMaterialInterface> _RibbonMaterial;

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

    /** One styled widget per POI entry. Unset: pooled plain UImages (_IconSize/_IconTint debug look). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_CompassUI_MarkerWidget> _MarkerWidgetClass;

    /** Debug icon markers — used only while _MarkerWidgetClass is unset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true, EditCondition = "_MarkerWidgetClass == nullptr"))
    FVector2D _IconSize = FVector2D{16.0, 16.0};

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon",
              meta = (AllowPrivateAccess = true, EditCondition = "_MarkerWidgetClass == nullptr"))
    FLinearColor _IconTint = FLinearColor::White;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Cardinals",
              meta = (AllowPrivateAccess = true))
    bool _ShowCardinals = true;

    /** Applied to the code-built cardinal labels when it names a valid font (leave unset for engine default). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Cardinals",
              meta = (AllowPrivateAccess = true, EditCondition = "_ShowCardinals"))
    FSlateFontInfo _CardinalFont;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Cardinals",
              meta = (AllowPrivateAccess = true, EditCondition = "_ShowCardinals"))
    FLinearColor _CardinalColor = FLinearColor::White;

    /** Normalized vertical band [0..1] the cardinal letters sit on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Ribbon|Cardinals",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0", EditCondition = "_ShowCardinals"))
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
    TObjectPtr<UCanvasPanel> _RootCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UImage> _FallbackRibbonImage;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _RibbonMID;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> _CardinalLabels;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UImage>> _IconPool;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_CompassUI_MarkerWidget>> _MarkerPool;

    bool _ReportedMisconfig = false;

public:
    CK_PROPERTY_GET(_Compass);
    CK_PROPERTY_GET(_Presentation);
    CK_PROPERTY_GET(_RibbonMaterial);
    CK_PROPERTY_GET(_HeadingParameterName);
    CK_PROPERTY_GET(_ArcParameterName);
    CK_PROPERTY_GET(_MarkerWidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------
