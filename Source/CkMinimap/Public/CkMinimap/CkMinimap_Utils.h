#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Delegates/CkDelegates.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkMinimap/CkMinimap_Fragment.h"
#include "CkMinimap/CkMinimap_Fragment_Data.h"

#include <NativeGameplayTags.h>

#include "CkMinimap_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// The PoiDisplayDefinition consumer tag the minimap projector resolves presentation (priority/offscreen) with.
CKMINIMAP_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_PoiConsumer_Minimap);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Minimap"))
class CKMINIMAP_API UCk_Utils_Minimap_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Minimap_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Minimap);

private:
    struct RecordOfMinimaps_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMinimaps> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Compose the Minimap feature DIRECTLY onto InHandle (no child entity). An entity hosts at most ONE direct
    // minimap; compose additional ones (HUD minimap AND fullscreen world map) as child entities via Create. The
    // observer defaults to InHandle's lifetime owner. InHandle does NOT need a Transform — only the observer does.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Add Feature")
    static FCk_Handle_Minimap
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Minimap_ParamsData& InParams);

    // Create a NEW Minimap child entity under InLifetimeOwner, connected to the owner's RecordOfMinimaps. The
    // observer defaults to InLifetimeOwner; redirect it via Request_SetObserver.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Create Feature")
    static FCk_Handle_Minimap
    Create(
        UPARAM(ref) FCk_Handle& InLifetimeOwner,
        const FCk_Fragment_Minimap_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Minimap",
        DisplayName="[Ck][Minimap] Has Any Minimap")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Minimap",
        DisplayName="[Ck][Minimap] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Minimap
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Minimap",
        DisplayName="[Ck][Minimap] Handle -> Minimap Handle",
        meta = (CompactNodeTitle = "<AsMinimap>", BlueprintAutocast))
    static FCk_Handle_Minimap
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Minimap Handle",
        Category = "Ck|Utils|Minimap",
        meta = (CompactNodeTitle = "INVALID_MinimapHandle", Keywords = "make"))
    static FCk_Handle_Minimap
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] For Each",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Minimap>
    ForEach_Minimap(
        const FCk_Handle& InMinimapOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Minimap(
        const FCk_Handle& InMinimapOwnerEntity,
        const TFunction<void(FCk_Handle_Minimap)>& InFunc) -> void;

public:
    // The per-frame pull surface: every projected POI as a self-contained value snapshot, sorted priority-desc
    // then distance-asc. Consumers SEED from this at bind time, then track deltas via the Appeared/Disappeared
    // signals — those never replay more than the LAST payload, so initial population must come from here.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Entries")
    static TArray<FCk_Minimap_Entry>
    Get_Entries(
        const FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Observer")
    static FCk_Handle
    Get_Observer(
        const FCk_Handle_Minimap& InMinimap);

    // Observer world position as of the LAST projection update (stales wholesale with _UpdateInterval)
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get View Origin")
    static FVector
    Get_ViewOrigin(
        const FCk_Handle_Minimap& InMinimap);

    // View yaw (degrees, [0, 360)) as of the LAST projection update. Under NorthLocked this is still
    // resolved and stored — widgets use it to rotate the observer arrow
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get View Yaw Degrees")
    static float
    Get_ViewYawDegrees(
        const FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get View Extent")
    static float
    Get_ViewExtent(
        const FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Projection Mode")
    static ECk_Minimap_ProjectionMode
    Get_ProjectionMode(
        const FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Rotation Mode")
    static ECk_Minimap_RotationMode
    Get_RotationMode(
        const FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Frame Shape")
    static ECk_Minimap_FrameShape
    Get_FrameShape(
        const FCk_Handle_Minimap& InMinimap);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Fixed Bounds")
    static FCk_Minimap_WorldBounds
    Get_FixedBounds(
        const FCk_Handle_Minimap& InMinimap);

    // Frame space -> world XY (map-click pings). Inverts whichever projection this minimap runs —
    // FixedBounds uses the bounds inverse, ObserverCentric the view inverse (as of the last update).
    // Returns world XY only: callers pick their own Z (trace or gameplay), never an invented one
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Get Frame To World")
    static FVector2D
    Get_FrameToWorld(
        const FCk_Handle_Minimap& InMinimap,
        FVector2D InFramePosition);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Request Set View Extent")
    static FCk_Handle_Minimap
    Request_SetViewExtent(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        float InViewExtent);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Request Set Category Filter")
    static FCk_Handle_Minimap
    Request_SetCategoryFilter(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        const FCk_Request_Minimap_SetCategoryFilter& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Request Set Observer")
    static FCk_Handle_Minimap
    Request_SetObserver(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        FCk_Handle InObserver);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Request Set Rotation Mode")
    static FCk_Handle_Minimap
    Request_SetRotationMode(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        ECk_Minimap_RotationMode InRotationMode);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName="[Ck][Minimap] Request Set Fog Of War")
    static FCk_Handle_Minimap
    Request_SetFogOfWar(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        FCk_Handle_FogOfWar InFogOfWar);

public:
    // Membership deltas ONLY (positions are per-frame data — pull those via Get_Entries/Get_ViewOrigin).
    // Default binding policy is IgnorePayloadInFlight on purpose: replay would deliver only the LAST
    // appeared entry — seed initial state from Get_Entries instead (see Get_Entries contract above).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName = "[Ck][Minimap] Bind To OnEntryAppeared")
    static FCk_Handle_Minimap
    BindTo_OnEntryAppeared(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryAppeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName = "[Ck][Minimap] Bind To OnEntryDisappeared")
    static FCk_Handle_Minimap
    BindTo_OnEntryDisappeared(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryDisappeared& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName = "[Ck][Minimap] Unbind From OnEntryAppeared")
    static FCk_Handle_Minimap
    UnbindFrom_OnEntryAppeared(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryAppeared& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Minimap",
              DisplayName = "[Ck][Minimap] Unbind From OnEntryDisappeared")
    static FCk_Handle_Minimap
    UnbindFrom_OnEntryDisappeared(
        UPARAM(ref) FCk_Handle_Minimap& InMinimap,
        const FCk_Delegate_Minimap_EntryDisappeared& InDelegate);

public:
    // Pure projection math — no entities, no world (unit-tested in CkMinimap_Utils.spec.cpp).
    // FRAME space is center-origin, +X = screen right, +Y = screen DOWN (UMG), visible frame [-1, 1]².
    // North-up mapping: Frame = (WorldDelta.Y / Extent, -WorldDelta.X / Extent).

    // NorthLocked ignores InViewYawDegrees; RotateWithObserver rotates by -InViewYawDegrees first.
    // Non-positive InViewExtent returns (0, 0).
    static auto
    Get_WorldToFrame(
        const FVector& InWorldPos,
        const FVector& InViewOrigin,
        float InViewYawDegrees,
        float InViewExtent,
        ECk_Minimap_RotationMode InRotationMode) -> FVector2D;

    // Inverse of Get_WorldToFrame. Returns world XY only — callers pick their own Z.
    static auto
    Get_FrameToWorld(
        const FVector2D& InFramePos,
        const FVector& InViewOrigin,
        float InViewYawDegrees,
        float InViewExtent,
        ECk_Minimap_RotationMode InRotationMode) -> FVector2D;

    // Projection over a fixed world rectangle, always north-up (the world-map). Invalid bounds return (0, 0).
    static auto
    Get_BoundsToFrame(
        const FVector& InWorldPos,
        const FCk_Minimap_WorldBounds& InBounds) -> FVector2D;

    // Inverse of Get_BoundsToFrame.
    static auto
    Get_FrameToWorld_Bounds(
        const FVector2D& InFramePos,
        const FCk_Minimap_WorldBounds& InBounds) -> FVector2D;

    // Edge-inclusive: |1| is INSIDE.
    static auto
    Get_IsInsideFrame(
        const FVector2D& InFramePos,
        ECk_Minimap_FrameShape InFrameShape) -> bool;

    // TOTAL: inside positions (and zero) return unchanged; outside positions pin onto the boundary
    // preserving direction (rect: / max(|x|, |y|); circle: / length).
    static auto
    Get_ClampToFrame(
        const FVector2D& InFramePos,
        ECk_Minimap_FrameShape InFrameShape) -> FVector2D;
};

// --------------------------------------------------------------------------------------------------------------------
