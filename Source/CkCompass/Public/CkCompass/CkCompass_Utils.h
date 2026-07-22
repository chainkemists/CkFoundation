#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkCompass/CkCompass_Fragment.h"
#include "CkCompass/CkCompass_Fragment_Data.h"

#include <NativeGameplayTags.h>

#include "CkCompass_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The PoiDisplayDefinition consumer tag the compass projector resolves presentation (priority/offscreen) with.
CKCOMPASS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_PoiConsumer_Compass);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Compass"))
class CKCOMPASS_API UCk_Utils_Compass_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Compass_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Compass);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Composes the Compass feature directly onto the given entity (typically the local player's pawn or
    // controller entity — one compass per local player). The observer defaults to this same entity;
    // redirect it via Request_SetObserver (e.g. onto a camera-rig entity).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Add Feature")
    static FCk_Handle_Compass
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Compass_ParamsData& InParams);

    // Create a NEW Compass child entity under InLifetimeOwner (Compass has no record — no record wiring). The
    // observer defaults to the created child; redirect it via Request_SetObserver.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Create Feature")
    static FCk_Handle_Compass
    Create(
        UPARAM(ref) FCk_Handle& InLifetimeOwner,
        const FCk_Fragment_Compass_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Compass",
        DisplayName="[Ck][Compass] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Compass
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Compass",
        DisplayName="[Ck][Compass] Handle -> Compass Handle",
        meta = (CompactNodeTitle = "<AsCompass>", BlueprintAutocast))
    static FCk_Handle_Compass
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Compass Handle",
        Category = "Ck|Utils|Compass",
        meta = (CompactNodeTitle = "INVALID_CompassHandle", Keywords = "make"))
    static FCk_Handle_Compass
    Get_InvalidHandle() { return {}; };

public:
    // Absolute world yaw of the compass heading this frame, degrees [0, 360). Refreshed every frame even
    // when the projection is interval-throttled — this is the value to feed a ribbon material's scalar param
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Get Heading")
    static float
    Get_Heading(
        const FCk_Handle_Compass& InCompass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Get Arc Degrees")
    static float
    Get_ArcDegrees(
        const FCk_Handle_Compass& InCompass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Get Observer")
    static FCk_Handle
    Get_Observer(
        const FCk_Handle_Compass& InCompass);

    // The per-frame pull surface: every projected POI as a self-contained value snapshot, sorted
    // priority-desc then distance-asc. Consumers seed their pooled widgets from this at bind time and
    // track membership deltas via the Appeared/Disappeared signals afterward — the signals never replay
    // more than the LAST payload, so initial population must come from here, never from signal replay.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Get Entries")
    static TArray<FCk_Compass_Entry>
    Get_Entries(
        const FCk_Handle_Compass& InCompass);

    // Pure 8-way helper for cardinal readouts (reuses CkCore's ECk_CardinalAndOrdinalDirection —
    // heading 0 = North = +X, 90 = East = +Y, 45-degree buckets centered on each direction)
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Get Cardinal Direction")
    static ECk_CardinalAndOrdinalDirection
    Get_CardinalDirection(
        float InHeadingDegrees);

public:
    // Pure arc math — no entities, no world (unit-tested in CkCompass_Utils.spec.cpp).

    // bearing / (arc/2), clamped to [-1, 1]. An unclamped |result| > 1 means the bearing is outside the arc.
    static auto
    Get_NormalizedArcOffset(
        float InSignedBearingDegrees,
        float InArcDegrees) -> float;

    // |bearing| > arc/2
    static auto
    Get_IsOutsideArc(
        float InSignedBearingDegrees,
        float InArcDegrees) -> bool;

    // Distance-based opacity for a projected POI: 1 inside the visible band, ramping to 0 across
    // InFadeBandCm INSIDE each active boundary (a range of 0 = that boundary is off; a band of
    // 0 = hard 1 everywhere). Callers cull out-of-range distances before calling — values outside
    // the band clamp to [0, 1].
    static auto
    Get_RangeFadeAlpha(
        float InDistance,
        float InMinVisibleRange,
        float InMaxVisibleRange,
        float InFadeBandCm) -> float;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Request Set Category Filter")
    static FCk_Handle_Compass
    Request_SetCategoryFilter(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        const FCk_Request_Compass_SetCategoryFilter& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Request Set Manual Heading")
    static FCk_Handle_Compass
    Request_SetManualHeading(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        float InHeadingDegrees);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName="[Ck][Compass] Request Set Observer")
    static FCk_Handle_Compass
    Request_SetObserver(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        FCk_Handle InObserver);

public:
    // Membership deltas ONLY (positions are per-frame data — pull those via Get_Entries/Get_Heading).
    // Default binding policy is IgnorePayloadInFlight on purpose: replay would deliver only the LAST
    // appeared entry — seed initial state from Get_Entries instead (see Get_Entries contract above).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName = "[Ck][Compass] Bind To OnEntryAppeared")
    static FCk_Handle_Compass
    BindTo_OnEntryAppeared(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryAppeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName = "[Ck][Compass] Bind To OnEntryDisappeared")
    static FCk_Handle_Compass
    BindTo_OnEntryDisappeared(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryDisappeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName = "[Ck][Compass] Unbind From OnEntryAppeared")
    static FCk_Handle_Compass
    UnbindFrom_OnEntryAppeared(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryAppeared& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Compass",
              DisplayName = "[Ck][Compass] Unbind From OnEntryDisappeared")
    static FCk_Handle_Compass
    UnbindFrom_OnEntryDisappeared(
        UPARAM(ref) FCk_Handle_Compass& InCompass,
        const FCk_Delegate_Compass_EntryDisappeared& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------