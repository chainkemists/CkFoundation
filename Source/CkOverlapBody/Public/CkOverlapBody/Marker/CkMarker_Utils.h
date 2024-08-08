#pragma once

#include "GameplayTagContainer.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkMarker_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Marker_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKOVERLAPBODY_API UCk_Utils_Marker_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Marker_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Marker);

private:
    struct RecordOfMarkers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMarkers> {};

public:
    friend class ck::FProcessor_Marker_Setup;
    friend class UCk_Utils_Ecs_Base_UE;
    friend class UCk_Utils_MarkerAndSensor_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Marker] Try Add New Marker")
    static FCk_Handle_Marker
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Marker_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType = ECk_Net_ReplicationType::All);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName="[Ck][Marker] Add Multiple New Markers")
    static TArray<FCk_Handle_Marker>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleMarker_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType = ECk_Net_ReplicationType::All);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Preview All",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAll(
        UObject* InOuter,
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Preview",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    Preview(
        UObject* InOuter,
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Marker",
        DisplayName="[Ck][Marker] Has Any Markers")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Marker",
        DisplayName="[Ck][Marker] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Marker
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Marker",
        DisplayName="[Ck][Marker] Handle -> Marker Handle",
        meta = (CompactNodeTitle = "<AsMarker>", BlueprintAutocast))
    static FCk_Handle_Marker
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="[Ck][Marker] Try Get Marker")
    static FCk_Handle_Marker
    TryGet_Marker(
        const FCk_Handle& InMarkerOwnerEntity,
        UPARAM(meta = (Categories = "Marker")) FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="[Ck][Marker] Try Get Entity With Marker In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_WithMarker_InOwnershipChain(
        const FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "Marker")) FGameplayTag InMarkerName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="[Ck][Marker] Get Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="[Ck][Marker] Get All Markers")
    static TArray<FCk_Handle_Marker>
    Get_All(
        FCk_Handle InMarkerOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Physics Info")
    static FCk_Marker_PhysicsInfo
    Get_PhysicsInfo(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Marker] Get Shape Info")
    static FCk_Marker_ShapeInfo
    Get_ShapeInfo(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Debug Info")
    static FCk_Marker_DebugInfo
    Get_DebugInfo(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Replication Type")
    static ECk_Net_ReplicationType
    Get_ReplicationType(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Relative Transform")
    static FTransform
    Get_RelativeTransform(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Relative Location")
    static FVector
    Get_RelativeLocation(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Relative Rotation")
    static FRotator
    Get_RelativeRotation(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Relative Scale")
    static FVector
    Get_RelativeScale(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get World Transform")
    static FTransform
    Get_WorldTransform(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get World Location")
    static FVector
    Get_WorldLocation(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get World Rotation")
    static FRotator
    Get_WorldRotation(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Get Shape Component")
    static UShapeComponent*
    Get_ShapeComponent(
        const FCk_Handle_Marker& InMarkerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Marker] Get Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    Get_AttachedEntityAndActor(
        const FCk_Handle_Marker& InMarkerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Marker] Request Enable/Disable")
    static FCk_Handle_Marker
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Marker& InMarkerEntity,
        const FCk_Request_Marker_EnableDisable& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Marker] Request Resize")
    static FCk_Handle_Marker
    Request_Resize(
        UPARAM(ref) FCk_Handle_Marker& InMarkerEntity,
        const FCk_Request_Marker_Resize& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Bind To OnEnableDisable")
    static void
    BindTo_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Marker& InMarkerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Unbind From OnEnableDisable")
    static void
    UnbindFrom_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Marker& InMarkerEntity,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate);

private:
    static auto
    DoPreviewMarker(
        UObject* InOuter,
        const FCk_Handle_Marker& InMarkerEntity) -> void;

    static auto
    Request_MarkMarker_AsNeedToUpdateTransform(
        FCk_Handle_Marker& InMarkerEntity) -> void;

    static auto
    Request_MarkMarker_AsSetupComplete(
        FCk_Handle_Marker& InMarkerEntity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
