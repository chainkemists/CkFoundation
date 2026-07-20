#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Delegates/CkDelegates.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkMinimap/CkMinimap_Fragment.h"
#include "CkMinimap/CkMinimap_Fragment_Data.h"

#include "CkMinimap_Utils.generated.h"

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
    // Compose a Minimap onto InHandle as a child entity. An entity may host MULTIPLE minimaps (a HUD
    // minimap AND a fullscreen world map are two children with different projection modes). The observer
    // defaults to InHandle (the child's lifetime owner); redirect it via Request_SetObserver. The owner
    // does NOT need a Transform — only the observer does.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Minimap] Add New Minimap")
    static FCk_Handle_Minimap
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Minimap_ParamsData& InParams);

public:
    // Has Feature
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
    // The per-frame pull surface: every projected POI as a self-contained value snapshot, sorted
    // priority-desc then distance-asc. Consumers seed their pooled widgets from this at bind time and
    // track membership deltas via the Appeared/Disappeared signals afterward — the signals never replay
    // more than the LAST payload, so initial population must come from here, never from signal replay.
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
};

// --------------------------------------------------------------------------------------------------------------------
