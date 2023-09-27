#pragma once

#include "GameplayTagContainer.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkOverlapBody/Marker/CkMarker_Fragment.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment_Params.h"
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

private:
    struct RecordOfMarkers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMarkers> {};

public:
    friend class ck::FProcessor_Marker_Setup;
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName="Add New Marker", meta = (GameplayTagFilter = "Marker"))
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Marker_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAllMarkers(
        UObject* InOuter,
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewMarker(
        UObject* InOuter,
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Has Marker")
    static bool
    Has(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Has Any Marker")
    static bool
    Has_Any(
        FCk_Handle InMarkerOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Ensure Has Marker")
    static bool
    Ensure(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Ensure Has Any Marker")
    static bool
    Ensure_Any(
        FCk_Handle InMarkerOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName="Get All Markers")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InMarkerOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Physics Info")
    static FCk_Marker_PhysicsInfo
    Get_PhysicsInfo(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Shape Info")
    static FCk_Marker_ShapeInfo
    Get_ShapeInfo(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Debug Info")
    static FCk_Marker_DebugInfo
    Get_DebugInfo(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Shape Component")
    static UShapeComponent*
    Get_ShapeComponent(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Marker",
              DisplayName = "Get Marker Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    Get_AttachedEntityAndActor(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Request Enable Disable Marker")
    static void
    Request_EnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Bind To Marker Enable Disable")
    static void
    BindTo_OnEnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Marker",
              DisplayName = "Unbind From Marker Enable Disable")
    static void
    UnbindFrom_OnEnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate);

private:
    static auto
    Has(
        FCk_Handle InHandle) -> bool;

    static auto
    DoPreviewMarker(
        UObject* InOuter,
        FCk_Handle InHandle) -> void;

    static auto
    Request_MarkMarker_AsNeedToUpdateTransform(
        FCk_Handle InMarkerHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
