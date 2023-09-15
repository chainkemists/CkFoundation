#include "CkSensor_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkNet/CkNet_Utils.h"
#include "CkOverlapBody/CkOverlapBody_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sensor_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Sensor_ParamsData& InParams)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle), TEXT("Cannot Add a Sensor to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return; }

    const auto& owningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle).Get_Actor().Get();
    const auto& sensorName = InParams.Get_SensorName();
    const auto& sensorReplicationType = InParams.Get_ReplicationType();
    const auto& outermostRemoteAuthority = [&]()
    {
        if (owningActor->GetIsReplicated())
        { return owningActor; }

        return UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(owningActor);
    }();

    if (NOT UCk_Utils_Net_UE::Get_IsRoleMatching(outermostRemoteAuthority, sensorReplicationType))
    {
        ck::overlap_body::VeryVerbose
        (
            TEXT("Skipping creation of Sensor [{}] on Actor [{}] because it's Replication Type [{}] does NOT match the Outermost Remote Autority [{}]"),
            sensorName,
            owningActor,
            sensorReplicationType,
            outermostRemoteAuthority
        );

        return;
    }

    auto sensorEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    auto sensorParams = InParams;
    sensorParams.Set_EntityAttachedTo(InHandle);

    sensorEntity.Add<ck::FFragment_Sensor_Params>(sensorParams);
    sensorEntity.Add<ck::FFragment_Sensor_Current>(sensorParams.Get_StartingState());
    sensorEntity.Add<ck::FTag_Sensor_Setup>();

    UCk_Utils_GameplayLabel_UE::Add(sensorEntity, sensorName);

    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfSensors_Utils::AddIfMissing(InHandle);

    RecordOfSensors_Utils::Request_Connect(InHandle, sensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return ck::IsValid(SensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle ,InSensorName), TEXT("Handle [{}] does NOT have a Sensor with Name [{}]"), InHandle, InSensorName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Sensor_UE::
    PreviewAllSensors(
        UObject*   InOuter,
        FCk_Handle InHandle)
    -> void
{
    if (NOT RecordOfSensors_Utils::Has(InHandle))
    { return; }

    RecordOfSensors_Utils::ForEachEntry(InHandle, [&](FCk_Handle InSensorEntity)
    {
        DoPreviewSensor(InOuter, InSensorEntity);
    });
}

auto
    UCk_Utils_Sensor_UE::
    PreviewSensor(
        UObject* InOuter,
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    DoPreviewSensor(InOuter, SensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    DoPreviewSensor(
        UObject* InOuter,
        FCk_Handle InHandle)
    -> void
{
    const auto& Params = InHandle.Get<ck::FFragment_Sensor_Params>();
    const auto& Current = InHandle.Get<ck::FFragment_Sensor_Current>();

    UCk_Utils_MarkerAndSensor_UE::Draw_Sensor_DebugLines(InOuter, Current, Params.Get_Params());
}

auto
    UCk_Utils_Sensor_UE::
    Get_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_PhysicsInfo
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_PhysicsParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_ShapeInfo
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_ShapeParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_DebugInfo
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_DebugParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> ECk_EnableDisable
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AllMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_MarkerOverlaps
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_CurrentMarkerOverlaps();
}

auto
    UCk_Utils_Sensor_UE::
    Get_HasMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return NOT Get_AllMarkerOverlaps(InHandle, InSensorName).Get_Overlaps().IsEmpty();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AllNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_NonMarkerOverlaps
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_CurrentNonMarkerOverlaps();
}

auto
    UCk_Utils_Sensor_UE::
    Get_HasNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return NOT Get_AllNonMarkerOverlaps(InHandle, InSensorName).Get_Overlaps().IsEmpty();
}

auto
    UCk_Utils_Sensor_UE::
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> UShapeComponent*
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_Sensor().Get();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_EntityOwningActor_BasicDetails
{
    if (NOT Ensure(InHandle, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_AttachedEntityAndActor();
}

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

    auto SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    SensorEntity.AddOrGet<ck::FFragment_Sensor_Requests>()._EnableDisableRequest = InRequest;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorEnableDisable::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorEnableDisable::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEndOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnBeginOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap_NonMarker::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnBeginOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap_NonMarker::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEndOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap_NonMarker::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEndOverlap_NonMarker(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InHandle, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InHandle, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap_NonMarker::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnBeginOverlap(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap& InRequest)
    -> void
{
    if (NOT Ensure(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    auto& requestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    requestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnEndOverlap(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnEndOverlap& InRequest)
    -> void
{
    if (NOT Ensure(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    auto& requestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    requestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnBeginOverlap_NonMarker(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest)
    -> void
{
    if (NOT Ensure(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    auto& requestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    requestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnEndOverlap_NonMarker(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest)
    -> void
{
    if (NOT Ensure(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    auto& requestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    requestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Sensor_Params, ck::FFragment_Sensor_Current>();
}

auto
    UCk_Utils_Sensor_UE::
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle InSensorHandle)
    -> void
{
    if (NOT Ensure(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    InSensorHandle.Add<ck::FTag_Sensor_UpdateTransform>();
}

// --------------------------------------------------------------------------------------------------------------------