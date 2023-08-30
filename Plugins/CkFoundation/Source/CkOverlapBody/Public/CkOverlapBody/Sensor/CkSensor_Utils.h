#pragma once

#include "CkLabel/CkLabel_Utils.h"
#include "CkMacros/CkMacros.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"

#include "CkOverlapBody/Sensor/CkSensor_Fragment.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSensor_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FCk_Processor_Sensor_Setup;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKOVERLAPBODY_API UCk_Utils_Sensor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Sensor_UE);

private:
    struct RecordOfSensors_Utils : public ck::TCk_Utils_Record<ck::FCk_Fragment_RecordOfSensors> {};

public:
    friend class ck::FCk_Processor_Sensor_Setup;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName="Add New Sensor", meta = (GameplayTagFilter = "Sensor"))
    static void
    Add(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Fragment_Sensor_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              meta = (DevelopmentOnly, DefaultToSelf = "InOuter"))
    static void
    PreviewAllSensors(
        UObject* InOuter,
        FCk_Handle InHandle);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="Has Sensor")
    static bool
    DoHas(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName="Ensure Has Sensor")
    static bool
    DoEnsure(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Physics Info")
    static FCk_Sensor_PhysicsInfo
    DoGet_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Shape Info")
    static FCk_Sensor_ShapeInfo
    DoGet_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Debug Info")
    static FCk_Sensor_DebugInfo
    DoGet_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Enable Disable")
    static ECk_EnableDisable
    DoGet_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Shape Component")
    static UShapeComponent*
    DoGet_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Sensor Attached Entity And Actor")
    static FCk_EntityOwningActor_BasicDetails
    DoGet_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Has Sensor Any Marker Overlaps")
    static bool
    DoGet_HasMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get All Sensor Marker Overlaps",
              meta = (AutoCreateRefTerm = "FCk_Handle"))
    static FCk_Sensor_MarkerOverlaps
    DoGet_AllMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get Has Sensor Any Non-Marker Overlaps")
    static bool
    DoGet_HasNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Get All Sensor Non-Marker Overlaps",
              meta = (AutoCreateRefTerm = "FCk_Handle"))
    static FCk_Sensor_NonMarkerOverlaps
    DoGet_AllNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Sensor",
              DisplayName = "Request Enable Disable Sensor")
    static void
    DoRequest_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest);

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
    // TODO: Bind functions

public:
    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Has(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> bool;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> bool;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> FCk_Sensor_PhysicsInfo;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> FCk_Sensor_ShapeInfo;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> FCk_Sensor_DebugInfo;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> ECk_EnableDisable;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> UShapeComponent*;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    PreviewSingleSensor(
        UObject* InOuter,
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> void;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InSensorName) -> FCk_EntityOwningActor_BasicDetails;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_HasMarkerOverlaps(
        FCk_Handle   InHandle,
        FGameplayTag InSensorName) -> bool;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_AllMarkerOverlaps(
        FCk_Handle   InHandle,
        FGameplayTag InSensorName) -> FCk_Sensor_MarkerOverlaps;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_HasNonMarkerOverlaps(
        FCk_Handle   InHandle,
        FGameplayTag InSensorName) -> bool;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Get_AllNonMarkerOverlaps(
        FCk_Handle   InHandle,
        FGameplayTag InSensorName) -> FCk_Sensor_NonMarkerOverlaps;

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy = ECk_FragmentQuery_Policy::EntityInRecord>
    static auto
    Request_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest) -> void;

private:
    static auto
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle InSensorHandle) -> void;

private:
    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
    static auto
    DoPerformCommonSensorUtilFunc(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        T_Func InFunc) -> decltype(InFunc(InHandle, InSensorName));

    template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
    static auto
    DoPerformCommonSensorUtilVoidFunc(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        T_Func InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag InSensorNameToUse)
    {
        return InSensorEntity.Has_All<ck::FCk_Fragment_Sensor_Params, ck::FCk_Fragment_Sensor_Current>() &&
               UCk_Utils_GameplayLabel_UE::Has(InSensorEntity) &&
               UCk_Utils_GameplayLabel_UE::Get_Label(InSensorEntity) == InSensorNameToUse;
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle ,InSensorName), TEXT("Handle [{}] does NOT have a Marker with Name [{}]"), InHandle, InSensorName)
    { return false; }

    return true;
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_PhysicsInfo
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Params>().Get_Params().Get_PhysicsParams();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_ShapeInfo
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Params>().Get_Params().Get_ShapeParams();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_DebugInfo
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Params>().Get_Params().Get_DebugParams();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> ECk_EnableDisable
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_EnableDisable();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> UShapeComponent*
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_Sensor().Get();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    PreviewSingleSensor(
        UObject* InOuter,
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    DoPerformCommonSensorUtilVoidFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        const auto& sensorParams = InSensorEntity.Get<ck::FCk_Fragment_Sensor_Params>().Get_Params();
        const auto& sensorCurrent = InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>();

        UCk_Utils_MarkerAndSensor_UE::Draw_Sensor_DebugLines(InOuter, sensorCurrent, sensorParams);
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_EntityOwningActor_BasicDetails
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_AttachedEntityAndActor();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_HasMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return NOT InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_CurrentMarkerOverlaps().Get_Overlaps().IsEmpty();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_AllMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_MarkerOverlaps
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_CurrentMarkerOverlaps();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_HasNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return NOT InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_CurrentNonMarkerOverlaps().Get_Overlaps().IsEmpty();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Get_AllNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_NonMarkerOverlaps
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    return DoPerformCommonSensorUtilFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        return InSensorEntity.Get<ck::FCk_Fragment_Sensor_Current>().Get_CurrentNonMarkerOverlaps();
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy>
auto
    UCk_Utils_Sensor_UE::
    Request_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    DoPerformCommonSensorUtilVoidFunc<T_FragmentQueryPolicy>(InHandle, InSensorName, [&](FCk_Handle InSensorEntity, FGameplayTag)
    {
        InSensorEntity.AddOrGet<ck::FCk_Fragment_Sensor_Requests>()._EnableDisableRequest = InRequest;
    });
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
auto
    UCk_Utils_Sensor_UE::
    DoPerformCommonSensorUtilFunc(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        T_Func InFunc)
    -> decltype(InFunc(InHandle, InSensorName))
{
    if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::CurrentEntity)
    {
        return InFunc(InHandle, InSensorName);
    }
    else if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::EntityInRecord)
    {
        const auto& MaybeSensor = RecordOfSensors_Utils::Get_RecordEntryIf(InHandle, [&](FCk_Handle InRecordEntry) -> bool
        {
            return Has<ECk_FragmentQuery_Policy::CurrentEntity>(InRecordEntry, InSensorName);
        });

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeSensor), TEXT("Entity [{}] does NOT have any Sensor with Name [{}]"), InHandle, InSensorName)
        { return {}; }

        return InFunc(MaybeSensor, InSensorName);
    }
    else
    {
        static_assert("Invalid Fragment Query policy");
        return {};
    }
}

template <ECk_FragmentQuery_Policy T_FragmentQueryPolicy, typename T_Func>
auto
    UCk_Utils_Sensor_UE::
    DoPerformCommonSensorUtilVoidFunc(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        T_Func InFunc)
    -> void
{
    if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::CurrentEntity)
    {
        InFunc(InHandle, InSensorName);
    }
    else if constexpr(T_FragmentQueryPolicy == ECk_FragmentQuery_Policy::EntityInRecord)
    {
        const auto& MaybeSensor = RecordOfSensors_Utils::Get_RecordEntryIf(InHandle, [&](FCk_Handle InRecordEntry) -> bool
        {
            return Has<ECk_FragmentQuery_Policy::CurrentEntity>(InRecordEntry, InSensorName);
        });

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeSensor), TEXT("Entity [{}] does NOT have any Sensor with Name [{}]"), InHandle, InSensorName)
        { return; }

        InFunc(MaybeSensor, InSensorName);
    }
    else
    {
        static_assert("Invalid Fragment Query policy");
    }
}

// --------------------------------------------------------------------------------------------------------------------
