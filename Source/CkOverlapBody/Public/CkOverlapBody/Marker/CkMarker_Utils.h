#pragma once

#include "GameplayTagContainer.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment_Data.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"

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
        FCk_Handle& InHandle,
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
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Marker",
        DisplayName="[Ck][Marker] Has Marker")
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
        FCk_Handle InHandle,
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
        FGameplayTag      InMarkerName);

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
              Category = "Ck|Utils|Marker",
              DisplayName = "[Ck][Marker] Request Enable/Disable")
    static FCk_Handle_Marker
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Marker& InMarkerEntity,
        const FCk_Request_Marker_EnableDisable& InRequest);

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
        UPARAM(ref) FCk_Handle_Marker& InMarkerEntity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
