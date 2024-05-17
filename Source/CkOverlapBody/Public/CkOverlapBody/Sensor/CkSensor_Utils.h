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
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Sensor);

private:
    struct RecordOfSensors_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfSensors> {};

public:
    friend class ck::FProcessor_Sensor_Setup;
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Sensor] Try Add New Sensor")
    static FCk_Handle_Sensor
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Sensor_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType = ECk_Net_ReplicationType::All);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Add Multiple New Sensors")
    static TArray<FCk_Handle_Sensor>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MultipleSensor_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType = ECk_Net_ReplicationType::All);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Preview All",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAll(
        UObject* InOuter,
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Preview",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    Preview(
        UObject* InOuter,
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Sensor",
        DisplayName="[Ck][Sensor] Has Any Sensors")
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
        Category = "Ck|Utils|Sensor",
        DisplayName="[Ck][Sensor] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Sensor
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Sensor",
        DisplayName="[Ck][Sensor] Handle -> Sensor Handle",
        meta = (CompactNodeTitle = "<AsSensor>", BlueprintAutocast))
    static FCk_Handle_Sensor
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Try Get Sensor")
    static FCk_Handle_Sensor
    TryGet_Sensor(
        const FCk_Handle& InSensorOwnerEntity,
        UPARAM(meta = (Categories = "Sensor")) FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Try Get Entity With Sensor In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_WithSensor_InOwnershipChain(
        const FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "Sensor")) FGameplayTag InSensorName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Get Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="[Ck][Sensor] Get All Sensors")
    static TArray<FCk_Handle_Sensor>
    Get_All(
        const FCk_Handle& InSensorOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Replication Type")
    static ECk_Net_ReplicationType
    Get_ReplicationType(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Physics Info")
    static FCk_Sensor_PhysicsInfo
    Get_PhysicsInfo(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Sensor] Get Shape Info")
    static FCk_Sensor_ShapeInfo
    Get_ShapeInfo(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Debug Info")
    static FCk_Sensor_DebugInfo
    Get_DebugInfo(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Enable Disable")
    static ECk_EnableDisable
    Get_EnableDisable(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Shape Component")
    static UShapeComponent*
    Get_ShapeComponent(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Sensor] Get Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    Get_AttachedEntityAndActor(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Has Any Marker Overlaps")
    static bool
    Get_HasMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Sensor] Get All Marker Overlaps")
    static FCk_Sensor_MarkerOverlaps
    Get_AllMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Marker Overlap Count")
    static int32
    Get_MarkerOverlapCount(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Has Any Non-Marker Overlaps")
    static bool
    Get_HasNonMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Sensor] Get All Non-Marker Overlaps")
    static FCk_Sensor_NonMarkerOverlaps
    Get_AllNonMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Get Non-Marker Overlap Count")
    static int32
    Get_NonMarkerOverlapCount(
        const FCk_Handle_Sensor& InSensorEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Sensor] Request Enable/Disable")
    static FCk_Handle_Sensor
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        const FCk_Request_Sensor_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnEnableDisable")
    static FCk_Handle_Sensor
    BindTo_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnEnableDisable")
    static FCk_Handle_Sensor
    UnbindFrom_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnBeginOverlap")
    static FCk_Handle_Sensor
    BindTo_OnBeginOverlap(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnBeginOverlap")
    static FCk_Handle_Sensor
    UnbindFrom_OnBeginOverlap(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnEndOverlap")
    static FCk_Handle_Sensor
    BindTo_OnEndOverlap(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnEndOverlap")
    static FCk_Handle_Sensor
    UnbindFrom_OnEndOverlap(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnBeginOverlap_NonMarker")
    static FCk_Handle_Sensor
    BindTo_OnBeginOverlap_NonMarker(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnBeginOverlap_NonMarker")
    static FCk_Handle_Sensor
    UnbindFrom_OnBeginOverlap_NonMarker(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Bind To OnEndOverlap_NonMarker")
    static FCk_Handle_Sensor
    BindTo_OnEndOverlap_NonMarker(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "[Ck][Sensor] Unbind From OnEndOverlap_NonMarker")
    static FCk_Handle_Sensor
    UnbindFrom_OnEndOverlap_NonMarker(
        UPARAM(ref) FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate);

public:
    static auto
    Request_OnBeginOverlap(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Request_Sensor_OnBeginOverlap& InRequest) -> void;

    static auto
    Request_OnEndOverlap(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Request_Sensor_OnEndOverlap& InRequest) -> void;

    static auto
    Request_OnBeginOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest) -> void;

    static auto
    Request_OnEndOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest) -> void;

private:
    static auto
    DoPreviewSensor(
        UObject* InOuter,
        const FCk_Handle_Sensor& InHandle) -> void;

    static auto
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle_Sensor& InSensorHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
