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
        FCk_Handle InHandle,
        const FCk_Fragment_Sensor_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle), TEXT("Cannot Add a Sensor to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return; }

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

        return;
    }

    auto ParamsToUse = InParams;
    ParamsToUse.Set_ReplicationType(InReplicationType);

    const auto NewSensorEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InSensorEntity)
    {
        InSensorEntity.Add<ck::FFragment_Sensor_Params>(ParamsToUse);
        InSensorEntity.Add<ck::FFragment_Sensor_Current>(ParamsToUse.Get_StartingState());
        InSensorEntity.Add<ck::FTag_Sensor_NeedsSetup>();

        UCk_Utils_GameplayLabel_UE::Add(InSensorEntity, SensorName);
    });

    RecordOfSensors_Utils::AddIfMissing(InHandle ,ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfSensors_Utils::Request_Connect(InHandle, NewSensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Has(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> bool
{
    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return ck::IsValid(SensorEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Has_Any(
        FCk_Handle InSensorOwnerEntity)
    -> bool
{
    return RecordOfSensors_Utils::Has(InSensorOwnerEntity);
}

auto
    UCk_Utils_Sensor_UE::
    Ensure_Any(
        FCk_Handle InSensorOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InSensorOwnerEntity), TEXT("Handle [{}] does NOT have any Sensor"), InSensorOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_Sensor_UE::
    Ensure(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InSensorOwnerEntity ,InSensorName), TEXT("Handle [{}] does NOT have a Sensor with Name [{}]"), InSensorOwnerEntity, InSensorName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Sensor_UE::
    PreviewAll(
        UObject*   InOuter,
        FCk_Handle InHandle)
    -> void
{
    if (NOT Has_Any(InHandle))
    { return; }

    RecordOfSensors_Utils::ForEach_ValidEntry(InHandle, [&](FCk_Handle InSensorEntity)
    {
        DoPreviewSensor(InOuter, InSensorEntity);
    });
}

auto
    UCk_Utils_Sensor_UE::
    Preview(
        UObject* InOuter,
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

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
    Get_ReplicationType(
        FCk_Handle   InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> ECk_Net_ReplicationType
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_ReplicationType();
}

auto
    UCk_Utils_Sensor_UE::
    Get_PhysicsInfo(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> FCk_Sensor_PhysicsInfo
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_PhysicsParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_ShapeInfo(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> FCk_Sensor_ShapeInfo
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_ShapeParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_DebugInfo(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> FCk_Sensor_DebugInfo
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Params>().Get_Params().Get_DebugParams();
}

auto
    UCk_Utils_Sensor_UE::
    Get_EnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> ECk_EnableDisable
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AllMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> FCk_Sensor_MarkerOverlaps
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_CurrentMarkerOverlaps();
}

auto
    UCk_Utils_Sensor_UE::
    Get_HasMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> bool
{
    return NOT Get_AllMarkerOverlaps(InSensorOwnerEntity, InSensorName).Get_Overlaps().IsEmpty();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AllNonMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> FCk_Sensor_NonMarkerOverlaps
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_CurrentNonMarkerOverlaps();
}

auto
    UCk_Utils_Sensor_UE::
    Get_HasNonMarkerOverlaps(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> bool
{
    return NOT Get_AllNonMarkerOverlaps(InSensorOwnerEntity, InSensorName).Get_Overlaps().IsEmpty();
}

auto
    UCk_Utils_Sensor_UE::
    Get_ShapeComponent(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> UShapeComponent*
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_Sensor().Get();
}

auto
    UCk_Utils_Sensor_UE::
    Get_AttachedEntityAndActor(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName)
    -> FCk_EntityOwningActor_BasicDetails
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return {}; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    return SensorEntity.Get<ck::FFragment_Sensor_Current>().Get_AttachedEntityAndActor();
}

auto
    UCk_Utils_Sensor_UE::
    Request_EnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Request_Sensor_EnableDisable& InRequest)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    auto SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    SensorEntity.AddOrGet<ck::FFragment_Sensor_Requests>()._EnableDisableRequest = InRequest;
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorEnableDisable::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEnableDisable& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorEnableDisable::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEndOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnBeginOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap_NonMarker::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnBeginOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnBeginOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorBeginOverlap_NonMarker::Unbind(SensorEntity, InDelegate);
}

auto
    UCk_Utils_Sensor_UE::
    BindTo_OnEndOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

    ck::UUtils_Signal_OnSensorEndOverlap_NonMarker::Bind(SensorEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Sensor_UE::
    UnbindFrom_OnEndOverlap_NonMarker(
        FCk_Handle InSensorOwnerEntity,
        FGameplayTag InSensorName,
        const FCk_Delegate_Sensor_OnEndOverlap_NonMarker& InDelegate)
    -> void
{
    if (NOT Ensure(InSensorOwnerEntity, InSensorName))
    { return; }

    const auto& SensorEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Sensor_UE,
        RecordOfSensors_Utils>(InSensorOwnerEntity, InSensorName);

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