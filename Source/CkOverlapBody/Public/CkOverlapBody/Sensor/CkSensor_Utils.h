#pragma once

#include "CkEcsBasics/CkEcsBasics_Utils.h"

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
              DisplayName="[Ck][Sensor] Add New Sensor")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Sensor_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType = ECk_Net_ReplicationType::All);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Preview All",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAll(
        UObject* InOuter,
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Preview",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    Preview(
        UObject* InOuter,
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Has Sensor")
    static bool
    Has(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Has Any Sensor")
    static bool
    Has_Any(
        FCk_Handle InSensorOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Ensure Has Sensor")
    static bool
    Ensure(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Ensure Has Any Sensor")
    static bool
    Ensure_Any(
        FCk_Handle InSensorOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Get All Sensors")
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InSensorOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Replication Type")
    static ECk_Net_ReplicationType
    Get_ReplicationType(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Physics Info")
    static FCk_Sensor_PhysicsInfo
    Get_PhysicsInfo(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Shape Info")
    static FCk_Sensor_ShapeInfo
    Get_ShapeInfo(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Debug Info")
    static FCk_Sensor_DebugInfo
    Get_DebugInfo(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Shape Component")
    static UShapeComponent*
    Get_ShapeComponent(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    Get_AttachedEntityAndActor(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Has Any Marker Overlaps")
    static bool
    Get_HasMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get All Marker Overlaps")
    static FCk_Sensor_MarkerOverlaps
    Get_AllMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Has Any Non-Marker Overlaps")
    static bool
    Get_HasNonMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get All Non-Marker Overlaps")
    static FCk_Sensor_NonMarkerOverlaps
    Get_AllNonMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Request Enable/Disable")
    static void
    Request_EnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnEnableDisable")
    static void
    BindTo_OnEnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnEnableDisable")
    static void
    UnbindFrom_OnEnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnBeginOverlap")
    static void
    BindTo_OnBeginOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnBeginOverlap")
    static void
    UnbindFrom_OnBeginOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnEndOverlap")
    static void
    BindTo_OnEndOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnEndOverlap")
    static void
    UnbindFrom_OnEndOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnBeginOverlap_NonMarker")
    static void
    BindTo_OnBeginOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnBeginOverlap_NonMarker")
    static void
    UnbindFrom_OnBeginOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnEndOverlap_NonMarker")
    static void
    BindTo_OnEndOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnEndOverlap_NonMarker")
    static void
    UnbindFrom_OnEndOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
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
    static auto
    DoPreviewSensor(
        UObject* InOuter,
        FCk_Handle InHandle) -> void;

    static auto
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle InSensorHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
