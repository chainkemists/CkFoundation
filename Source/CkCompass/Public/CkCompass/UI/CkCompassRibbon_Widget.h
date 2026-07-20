#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCompass/CkCompass_Fragment_Data.h"
#include "CkCompass/CkCompass_Utils.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCompassRibbon_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCanvasPanel;
class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UMaterialInterface;

// --------------------------------------------------------------------------------------------------------------------

// Reference consumer of the CkCompass delivery contract, fully self-contained (ZERO asset dependencies):
// cardinal letters are pooled TextBlocks, POI markers are pooled Images, both positioned in ONE loop per
// frame from the pull API (Get_Heading + Get_Entries). An optional ribbon material receives the heading
// as a scalar parameter for GPU-drawn tick strips.
//
// NOTE on the pull-in-NativeTick shape: CkUI doctrine routes per-frame widget updates through processors
// (see WorldSpaceWidget). This widget is the DOCUMENTED EDGE-CONSUMER of the compass pull API — the math
// is precomputed by FProcessor_Compass_Update; this tick only writes UMG layout for its own pooled
// children. Games wanting processor-pushed HUDs can consume the same API from their own processor.
//
// The Appeared/Disappeared signals are bound (IgnorePayloadInFlight) and surfaced as Blueprint events to
// demonstrate the membership-delta contract; positioning does NOT depend on them (index-pooled per frame).
//
// [EDITOR-VERIFY] The base widget class carries meta=(DisableNativeTick); class metadata is not inherited
// and is editor-only, so this subclass is expected to tick — verify NativeTick actually runs in PIE on
// first integration; if it does not, drive Refresh from a timer or remove the base meta expectation.
UCLASS(Blueprintable, BlueprintType)
class CKCOMPASS_API UCk_CompassRibbon_Widget : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CompassRibbon_Widget);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Compass",
              DisplayName = "[Ck][Compass] Set Compass")
    void
    Set_Compass(
        UPARAM(ref) FCk_Handle_Compass& InCompass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|Compass",
              DisplayName = "[Ck][Compass] Get Compass")
    FCk_Handle_Compass
    Get_Compass() const;

protected:
    // Membership deltas, forwarded from the compass signals (bound with IgnorePayloadInFlight — initial
    // population comes from Get_Entries inside Set_Compass, per the delivery contract)
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Compass")
    void
    OnCompassEntryAppeared(
        const FCk_Compass_Entry& InEntry);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Compass")
    void
    OnCompassEntryDisappeared(
        const FCk_Handle_Poi& InPoi);

protected:
    auto NativeOnInitialized() -> void override;
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
    auto DoBuildWidgetTree() -> void;
    auto DoRefreshLayout(const FGeometry& InGeometry) -> void;
    auto DoPositionChildAtOffset(UWidget* InChild, float InNormalizedOffset, const FVector2D& InPanelSize, float InVerticalCenter) const -> void;

private:
    // Optional — a ribbon/tick-strip material; receives the heading (degrees, 0-360) on _HeadingParameterName
    // every frame via a dynamic instance. Leave unset for the pure-widget presentation.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _RibbonMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass",
              meta = (AllowPrivateAccess = true))
    FName _HeadingParameterName = TEXT("Heading");

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass",
              meta = (AllowPrivateAccess = true))
    FVector2D _IconSize = FVector2D{16.0, 16.0};

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass",
              meta = (AllowPrivateAccess = true))
    FLinearColor _IconTint = FLinearColor::White;

    // Clamped-to-edge entries render with this opacity multiplier so off-arc waypoints read as pinned
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _ClampedEntryOpacity = 0.5f;

private:
    UPROPERTY(Transient)
    FCk_Handle_Compass _Compass;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> _RootCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UImage> _RibbonImage;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _RibbonMID;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> _CardinalLabels;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UImage>> _IconPool;

public:
    CK_PROPERTY_GET(_RibbonMaterial);
    CK_PROPERTY_GET(_HeadingParameterName);
};

// --------------------------------------------------------------------------------------------------------------------