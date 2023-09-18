#pragma once

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"
#include "CkOverlapBody/Sensor/CkSensor_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkSensor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Sensor_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKOVERLAPBODY_API UCk_Utils_Sensor_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Sensor_UE);

private:
    struct RecordOfSensors_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSensors> {};

public:
    friend class ck::FProcessor_Sensor_Setup;
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName="Add New Sensor", meta = (GameplayTagFilter = "Sensor"))
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Sensor_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAllSensors(
        UObject* InOuter,
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewSensor(
        UObject* InOuter,
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

private:
    static auto
    DoPreviewSensor(
        UObject* InOuter,
        FCk_Handle InHandle) -> void;

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="Has Sensor")
    static bool
    Has(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="Ensure Has Sensor")
    static bool
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Physics Info")
    static FCk_Sensor_PhysicsInfo
    Get_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Shape Info")
    static FCk_Sensor_ShapeInfo
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Debug Info")
    static FCk_Sensor_DebugInfo
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Shape Component")
    static UShapeComponent*
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Has Sensor Any Marker Overlaps")
    static bool
    Get_HasMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get All Sensor Marker Overlaps",
              meta = (AutoCreateRefTerm = "FCk_Handle"))
    static FCk_Sensor_MarkerOverlaps
    Get_AllMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Has Sensor Any Non-Marker Overlaps")
    static bool
    Get_HasNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get All Sensor Non-Marker Overlaps",
              meta = (AutoCreateRefTerm = "FCk_Handle"))
    static FCk_Sensor_NonMarkerOverlaps
    Get_AllNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Request Enable Disable Sensor")
    static void
    Request_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Bind To Sensor Enable Disable")
    static void
    BindTo_OnEnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Unbind From Sensor Enable Disable")
    static void
    UnbindFrom_OnEnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Bind To Sensor Begin Overlap")
    static void
    BindTo_OnBeginOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Unbind From Sensor Begin Overlap")
    static void
    UnbindFrom_OnBeginOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Bind To Sensor End Overlap")
    static void
    BindTo_OnEndOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Unbind From Sensor End Overlap")
    static void
    UnbindFrom_OnEndOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Bind To Sensor Begin Overlap Non-Marker")
    static void
    BindTo_OnBeginOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Unbind From Sensor Begin Overlap Non-Marker")
    static void
    UnbindFrom_OnBeginOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Bind To Sensor End Overlap Non-Marker")
    static void
    BindTo_OnEndOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Unbind From Sensor End Overlap Non-Marker")
    static void
    UnbindFrom_OnEndOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate);

public:
    static auto
    Request_OnBeginOverlap(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap& InRequest) -> void;

    static auto
    Request_OnEndOverlap(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnEndOverlap& InRequest) -> void;

    static auto
    Request_OnBeginOverlap_NonMarker(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest) -> void;

    static auto
    Request_OnEndOverlap_NonMarker(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest) -> void;

private:
    static auto
    Has(
        FCk_Handle InSensorHandle) -> bool;

private:
    static auto
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle InSensorHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
