#include "CkSensor_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkNet/CkNet_Utils.h"
#include "CkOverlapBody/CkOverlapBody_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sensor_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Sensor_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> FCk_Handle_Sensor
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Cannot Add a Sensor to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return {}; }

    const auto& SensorName = InParams.Get_SensorName();

    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InHandle, InReplicationType))
    {
        ck::overlap_body::VeryVerbose
        (
            TEXT("Skipping creation of Sensor [{}] on Entity [{}] because it's Replication Type [{}] does NOT match"),
            SensorName,
            InHandle,
            InReplicationType
        );

        return {};
    }

    auto ParamsToUse = InParams;
    ParamsToUse.Set_ReplicationType(InReplicationType);

    auto NewSensorEntity = CastChecked(UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle,
    [&](FCk_Handle InSensorEntity)
    {
        InSensorEntity.Add<ck::FFragment_Sensor_Params>(ParamsToUse);
        InSensorEntity.Add<ck::FFragment_Sensor_Current>(ParamsToUse.Get_StartingState());
        InSensorEntity.Add<ck::FTag_Sensor_NeedsSetup>();

        UCk_Utils_GameplayLabel_UE::Add(InSensorEntity, SensorName);
    }));

    RecordOfSensors_Utils::AddIfMissing(InHandle ,ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfSensors_Utils::Request_Connect(InHandle, NewSensorEntity);

    return NewSensorEntity;
}


auto
    UCk_Utils_Sensor_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleSensor_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> TArray<FCk_Handle_Sensor>
{
    return ck::algo::Transform<TArray<FCk_Handle_Sensor>>(InParams.Get_SensorParams(),
    [&](const FCk_Fragment_Sensor_ParamsData& InSensorParams)
    {
        return Add(InHandle, InSensorParams, InReplicationType);
    });
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Sensor, UCk_Utils_Sensor_UE, FCk_Handle_Sensor, ck::FFragment_Sensor_Params, ck::FFragment_Sensor_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sensor_UE::
    TryGet_Sensor(
        const FCk_Handle& InSensorOwnerEntity,
        FGameplayTag      InSensorName)
    -> FCk_Handle_Sensor
{
    if (NOT RecordOfSensors_Utils::Has(InSensorOwnerEntity))
    { return {}; }

    return RecordOfSensors_Utils::Get_ValidEntry_If(InSensorOwnerEntity, ck::algo::MatchesGameplayLabelExact{InSensorName});
}

auto
    UCk_Utils_Sensor_UE::
    Get_Name(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InSensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Get_All(
        const FCk_Handle& InSensorOwnerEntity)
    -> TArray<FCk_Handle_Sensor>
{
    if (NOT Has_Any(InSensorOwnerEntity))
    { return {}; }

    auto AllSensors = TArray<FCk_Handle_Sensor>{};

    RecordOfSensors_Utils::ForEach_ValidEntry(InSensorOwnerEntity, [&](FCk_Handle_Sensor InSensorEntity)
    {
        AllSensors.Emplace(InSensorEntity);
    });

    return AllSensors;
}

auto
    UCk_Utils_Sensor_UE::
    PreviewAll(
        UObject* InOuter,
        const FCk_Handle& InHandle)
    -> void
{
    if (NOT Has_Any(InHandle))
    { return; }

    RecordOfSensors_Utils::ForEach_ValidEntry(InHandle, [&](const FCk_Handle_Sensor& InSensorEntity)
    {
        DoPreviewSensor(InOuter, InSensorEntity);
    });
}

auto
    UCk_Utils_Sensor_UE::
    Preview(
        UObject* InOuter,
        const FCk_Handle_Sensor& InSensorEntity)
    -> void
{
    DoPreviewSensor(InOuter, InSensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfSensors_Utils::Has(InHandle);
}

auto
    UCk_Utils_Sensor_UE::
    Get_ReplicationType(
        const FCk_Handle_Sensor& InSensorEntity)
    -> ECk_Net_ReplicationType
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_ReplicationType();
}

auto
    UCk_Utils_Sensor_UE::
    Get_PhysicsInfo(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FCk_Sensor_PhysicsInfo
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_PhysicsParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_ShapeInfo(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FCk_Sensor_ShapeInfo
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_ShapeParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_DebugInfo(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FCk_Sensor_DebugInfo
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_DebugParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_EnableDisable(
        const FCk_Handle_Sensor& InSensorEntity)
    -> ECk_EnableDisable
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AllMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FCk_Sensor_MarkerOverlaps
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Current>().Get_CurrentMarkerOverlaps();
}

auto
    UCk_Utils_Sensor_UE::
    Get_HasMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity)
    -> bool
{
    return NOT Get_AllMarkerOverlaps(InSensorEntity).Get_Overlaps().IsEmpty();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AllNonMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FCk_Sensor_NonMarkerOverlaps
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Current>().Get_CurrentNonMarkerOverlaps();
}

auto
    UCk_Utils_Sensor_UE::
    Get_HasNonMarkerOverlaps(
        const FCk_Handle_Sensor& InSensorEntity)
    -> bool
{
    return NOT Get_AllNonMarkerOverlaps(InSensorEntity).Get_Overlaps().IsEmpty();
}

auto
    UCk_Utils_Sensor_UE::
    Get_ShapeComponent(
        const FCk_Handle_Sensor& InSensorEntity)
    -> UShapeComponent*
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Current>().Get_Sensor().Get();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AttachedEntityAndActor(
        const FCk_Handle_Sensor& InSensorEntity)
    -> FCk_EntityOwningActor_BasicDetails
{
    return InSensorEntity.Get<ck::FFragment_Sensor_Current>().Get_AttachedEntityAndActor();
}

auto
    UCk_Utils_Sensor_UE::
    Request_EnableDisable(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Request_Sensor_EnableDisable& InRequest)
    -> FCk_Handle_Sensor
{
    InSensorEntity.AddOrGet<ck::FFragment_Sensor_Requests>()._EnableDisableRequest = InRequest;

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEnableDisable(
        FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorEnableDisable::Bind(InSensorEntity, InDelegate, InBindingPolicy);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorEnableDisable::Unbind(InSensorEntity, InDelegate);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorBeginOverlap::Bind(InSensorEntity, InDelegate, InBindingPolicy);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorBeginOverlap::Unbind(InSensorEntity, InDelegate);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEndOverlap(
        FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorEndOverlap::Bind(InSensorEntity, InDelegate, InBindingPolicy);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorEndOverlap::Unbind(InSensorEntity, InDelegate);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnBeginOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorBeginOverlap_NonMarker::Bind(InSensorEntity, InDelegate, InBindingPolicy);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnBeginOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorBeginOverlap_NonMarker::Unbind(InSensorEntity, InDelegate);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEndOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorEndOverlap_NonMarker::Bind(InSensorEntity, InDelegate, InBindingPolicy);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEndOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorEntity,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate)
    -> FCk_Handle_Sensor
{
    ck::UUtils_Signal_OnSensorEndOverlap_NonMarker::Unbind(InSensorEntity, InDelegate);

    return InSensorEntity;
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnBeginOverlap(
        FCk_Handle_Sensor& InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap& InRequest)
    -> void
{
    auto& RequestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    RequestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnEndOverlap(
        FCk_Handle_Sensor& InSensorHandle,
        const FCk_Request_Sensor_OnEndOverlap& InRequest)
    -> void
{
    auto& RequestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    RequestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnBeginOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorHandle,
        const FCk_Request_Sensor_OnBeginOverlap_NonMarker& InRequest)
    -> void
{
    auto& RequestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    RequestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    Request_OnEndOverlap_NonMarker(
        FCk_Handle_Sensor& InSensorHandle,
        const FCk_Request_Sensor_OnEndOverlap_NonMarker& InRequest)
    -> void
{
    auto& RequestsComp = InSensorHandle.AddOrGet<ck::FFragment_Sensor_Requests>();
    RequestsComp._BeginOrEndOverlapRequests.Emplace(InRequest);
}

auto
    UCk_Utils_Sensor_UE::
    DoPreviewSensor(
        UObject* InOuter,
        const FCk_Handle_Sensor& InHandle)
    -> void
{
    const auto& Params = InHandle.Get<ck::FFragment_Sensor_Params>();
    const auto& Current = InHandle.Get<ck::FFragment_Sensor_Current>();

    UCk_Utils_MarkerAndSensor_UE::Draw_Sensor_DebugLines(InOuter, Current, Params.Get_Params());
}


auto
    UCk_Utils_Sensor_UE::
    Request_MarkSensor_AsNeedToUpdateTransform(
        FCk_Handle_Sensor& InSensorHandle)
    -> void
{
    InSensorHandle.Add<ck::FTag_Sensor_UpdateTransform>();
}

// --------------------------------------------------------------------------------------------------------------------