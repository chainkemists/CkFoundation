#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCompass/CkCompass_Fragment_Data.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Fragment_Data.h"

#include "CkUICore/UserWidget/CkUserWidget.h"

#include "CkCompassUI_MarkerWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UImage;
class UTextBlock;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-POI marker for the compass ribbon. The ribbon owns positioning; this widget only styles itself:
 * the POI's DisplayDefinition (icon/tint/size hint) is resolved natively into the optional bound _Icon,
 * the distance readout is formatted into the optional bound _DistanceText, and OnEntryUpdated forwards
 * the full entry for anything bespoke. Author a WBP subclass with ZERO graph — bind either element
 * (or neither) and style. Display resolution only re-runs when the pooled slot is reassigned to a
 * different POI; the distance SetText only fires when the rounded meter count changes.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKCOMPASS_API UCk_CompassUI_MarkerWidget : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CompassUI_MarkerWidget);

private:
    /** The POI this pooled marker currently represents. */
    UPROPERTY(Transient, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Poi _AssignedPoi;

    /** Optional icon image. Name a UImage "_Icon" in the marker Blueprint to opt in — the POI's
     *  DisplayDefinition (icon texture, tint, size hint) is resolved into it natively. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> _Icon;

    /** Optional distance readout. Name a UTextBlock "_DistanceText" in the marker Blueprint to opt in. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _DistanceText;

public:
    CK_PROPERTY_GET(_AssignedPoi);

public:
    /** Called by the ribbon on every projection refresh (and at design time with a preview entry —
     *  the preview entry's Poi handle is invalid, so designer-authored icon defaults are left as-is). */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Compass|Marker",
              DisplayName = "Inject Compass Entry")
    void
    InjectEntry(
        const FCk_Compass_Entry& InEntry);

    /** Called by the ribbon on the frame this pooled marker is laid out for a POI that JUST appeared on
     *  the compass (a genuine membership-add). NOT fired for the seed set, per-frame refreshes, or a
     *  pool index reshuffle — the one moment to play an attention "pop". */
    void
    NotifyShown();

protected:
    /** Implement in Blueprint for anything bespoke (elevation arrows, priority badges, category styling). */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Compass|Marker")
    void
    OnEntryUpdated(
        const FCk_Compass_Entry& InEntry);

    /** Implement in Blueprint / AngelScript to react when this marker's POI newly appears (play a scale
     *  "pop", flash, etc.). Fires once per appearance, not every layout refresh. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Compass|Marker")
    void
    OnMarkerShown();

protected:
    // Design-time: self-injects a preview entry so the marker WBP previews its own distance
    // readout while being styled (the preview entry's Poi is invalid — icon defaults are kept)
    auto NativePreConstruct() -> void override;

    // Drops the display-change binding — a pooled marker outlives any single POI, and the ribbon may
    // destroy it while it is still bound.
    auto NativeDestruct() -> void override;

private:
    auto DoResolveDisplay() -> void;
    // Full apply, for POI assignment: resolves the immutable icon, then the mutable state.
    auto DoApplyDisplay(const FCk_Handle_PoiDisplayDefinition& InDefinition) -> void;
    // Tint + size only — everything a display-changed broadcast can actually have altered.
    auto DoApplyMutableDisplay(const FCk_Handle_PoiDisplayDefinition& InDefinition) -> void;
    auto DoUnbindDisplayChanged() -> void;
    auto DoCaptureAuthoredDisplay() -> void;
    auto DoRestoreAuthoredDisplay() -> void;

    // Re-reads the definition's Current state when a per-instance override lands (tint or size).
    UFUNCTION()
    void
    DoOnDisplayChanged(
        FCk_Handle_PoiDisplayDefinition InDefinition);

private:
    /** The display definition this marker is currently bound to. Pooled markers get reassigned across POIs,
     *  so the previous binding is dropped before a new one is taken. */
    UPROPERTY(Transient)
    FCk_Handle_PoiDisplayDefinition _BoundDisplayDefinition;

    /** Whatever the marker Blueprint authored on _Icon, captured lazily the first time it is about to be
     *  overwritten. Markers are POOLED, so a slot recycled onto a POI with no compass definition would
     *  otherwise keep the PREVIOUS POI's icon and tint. UPROPERTY because FSlateBrush holds a
     *  ResourceObject that must stay GC-reachable. */
    UPROPERTY(Transient)
    FSlateBrush _AuthoredBrush;

    UPROPERTY(Transient)
    FLinearColor _AuthoredTint = FLinearColor::White;

    bool _AuthoredCaptured = false;

private:
    /** When > 0, _DistanceText is collapsed while the entry is NEARER than this (cm) — no "1 m"
     *  readout while standing on the POI. 0 = always show; the widget then never touches the
     *  text's authored visibility. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Marker",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _DistanceTextVisibleBeyondCm = 0.0f;

    /** Distance (cm) shown by the designer preview. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Compass|Marker|Preview",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _PreviewDistanceCm = 2500.0f;

    int32 _LastDistanceMeters = -1;
};

// --------------------------------------------------------------------------------------------------------------------
