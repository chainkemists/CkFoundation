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
    DoHas(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return Has(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoEnsure(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return Ensure(InHandle, InSensorName);
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
        const auto& sensorName = InSensorEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_MarkerName();
        PreviewSingleSensor<ECk_FragmentQuery_Policy::CurrentEntity>(InOuter, InSensorEntity, sensorName);
    });
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_PhysicsInfo
{
    return Get_PhysicsInfo(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_ShapeInfo
{
    return Get_ShapeInfo(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_DebugInfo
{
    return Get_DebugInfo(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> ECk_EnableDisable
{
    return Get_EnableDisable(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_AllMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_MarkerOverlaps
{
    return Get_AllMarkerOverlaps(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_HasMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return Get_HasMarkerOverlaps(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_AllNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_Sensor_NonMarkerOverlaps
{
    return Get_AllNonMarkerOverlaps(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_HasNonMarkerOverlaps(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> bool
{
    return Get_HasNonMarkerOverlaps(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> UShapeComponent*
{
    return Get_ShapeComponent(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoGet_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InSensorName)
    -> FCk_EntityOwningActor_BasicDetails
{
    return Get_AttachedEntityAndActor(InHandle, InSensorName);
}

auto
    UCk_Utils_Sensor_UE::
    DoRequest_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest)
    -> void
{
    Request_EnableDisable(InHandle, InSensorName, InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnBeginOverlap(
        FCk_Handle InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap& InRequest)
    -> void
{
    if (NOT Ensure<ECk_FragmentQuery_Policy::CurrentEntity>(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
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
    if (NOT Ensure<ECk_FragmentQuery_Policy::CurrentEntity>(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
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
    if (NOT Ensure<ECk_FragmentQuery_Policy::CurrentEntity>(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
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
    if (NOT Ensure<ECk_FragmentQuery_Policy::CurrentEntity>(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    auto& requestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    requestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle InSensorHandle)
    -> void
{
    if (NOT Ensure<ECk_FragmentQuery_Policy::CurrentEntity>(InSensorHandle, UCk_Utils_GameplayLabel_UE::Get_Label(InSensorHandle)))
    { return; }

    InSensorHandle.Add<ck::FTag_Sensor_UpdateTransform>();
}

// --------------------------------------------------------------------------------------------------------------------