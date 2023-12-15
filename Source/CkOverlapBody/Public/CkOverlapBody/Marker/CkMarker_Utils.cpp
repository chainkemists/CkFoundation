#include "CkMarker_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkNet/CkNet_Utils.h"
#include "CkOverlapBody/CkOverlapBody_Log.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Marker_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Marker_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle), TEXT("Cannot Add a Marker to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return; }

    const auto& markerName = InParams.Get_MarkerName();

    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InHandle, InReplicationType))
    {
        ck::overlap_body::VeryVerbose
        (
            TEXT("Skipping creation of Marker [{}] because it's Replication Type [{}] does NOT match"),
            markerName,
            InReplicationType
        );

        return;
    }

    auto ParamsToUse = InParams;
    ParamsToUse.Set_ReplicationType(InReplicationType);

    const auto NewMarkerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InMarkerEntity)
    {
        InMarkerEntity.Add<ck::FFragment_Marker_Params>(ParamsToUse);
        InMarkerEntity.Add<ck::FFragment_Marker_Current>(ParamsToUse.Get_StartingState());
        InMarkerEntity.Add<ck::FTag_Marker_NeedsSetup>();

        UCk_Utils_GameplayLabel_UE::Add(InMarkerEntity, markerName);
    });

    RecordOfMarkers_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfMarkers_Utils::Request_Connect(InHandle, NewMarkerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Has(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> bool
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return ck::IsValid(MarkerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Has_Any(
        FCk_Handle InMarkerOwnerEntity)
    -> bool
{
    return RecordOfMarkers_Utils::Has(InMarkerOwnerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Ensure_Any(
        FCk_Handle InMarkerOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InMarkerOwnerEntity), TEXT("Handle [{}] does NOT have any Marker"), InMarkerOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_Marker_UE::
    Ensure(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InMarkerOwnerEntity ,InMarkerName), TEXT("Handle [{}] does NOT have a Marker with Name [{}]"), InMarkerOwnerEntity, InMarkerName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Marker_UE::
    Get_All(
        FCk_Handle InMarkerOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT Has_Any(InMarkerOwnerEntity))
    { return {}; }

    auto AllMarkers = TArray<FGameplayTag>{};

    RecordOfMarkers_Utils::ForEach_ValidEntry(InMarkerOwnerEntity, [&](FCk_Handle InMarkerEntity)
    {
        AllMarkers.Emplace(UCk_Utils_GameplayLabel_UE::Get_Label(InMarkerEntity));
    });

    return AllMarkers;
}

auto
    UCk_Utils_Marker_UE::
    PreviewAllMarkers(
        UObject*   InOuter,
        FCk_Handle InHandle)
    -> void
{
    if (NOT Has_Any(InHandle))
    { return; }

    RecordOfMarkers_Utils::ForEach_ValidEntry(InHandle, [&](FCk_Handle InMarkerEntity)
    {
        DoPreviewMarker(InOuter, InMarkerEntity);
    });
}

auto
    UCk_Utils_Marker_UE::
    PreviewMarker(
        UObject*     InOuter,
        FCk_Handle   InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> void
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    DoPreviewMarker(InOuter, MarkerEntity);
}

auto
    UCk_Utils_Marker_UE::
    DoPreviewMarker(
        UObject* InOuter,
        FCk_Handle InHandle)
    -> void
{
    const auto& Params = InHandle.Get<ck::FFragment_Marker_Params>();
    const auto& Current = InHandle.Get<ck::FFragment_Marker_Current>();

    UCk_Utils_MarkerAndSensor_UE::Draw_Marker_DebugLines(InOuter, Current, Params.Get_Params());
}

auto
    UCk_Utils_Marker_UE::
    Get_PhysicsInfo(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> FCk_Marker_PhysicsInfo
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_PhysicsParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_ShapeInfo(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> FCk_Marker_ShapeInfo
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_ShapeParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_DebugInfo(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> FCk_Marker_DebugInfo
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_DebugParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_ReplicationType(
        FCk_Handle   InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> ECk_Net_ReplicationType
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_ReplicationType();
}

auto
    UCk_Utils_Marker_UE::
    Get_EnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> ECk_EnableDisable
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Marker_UE::
    Get_ShapeComponent(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName)
    -> UShapeComponent*
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Current>().Get_Marker().Get();
}

auto
    UCk_Utils_Marker_UE::
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_EntityOwningActor_BasicDetails
{
    if (NOT Ensure(InHandle, InMarkerName))
    { return {}; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Current>().Get_AttachedEntityAndActor();
}

auto
    UCk_Utils_Marker_UE::
    Request_EnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest)
    -> void
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return; }

    auto MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    MarkerEntity.AddOrGet<ck::FFragment_Marker_Requests>()._Requests = InRequest;
}

auto
    UCk_Utils_Marker_UE::
    BindTo_OnEnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate)
    -> void
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    ck::UUtils_Signal_OnMarkerEnableDisable::Bind(MarkerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Marker_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle InMarkerOwnerEntity,
        FGameplayTag InMarkerName,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate)
    -> void
{
    if (NOT Ensure(InMarkerOwnerEntity, InMarkerName))
    { return; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerOwnerEntity, InMarkerName);

    ck::UUtils_Signal_OnMarkerEnableDisable::Unbind(MarkerEntity, InDelegate);
}

auto
    UCk_Utils_Marker_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_Marker_Params, ck::FFragment_Marker_Current>();
}

auto
    UCk_Utils_Marker_UE::
    Request_MarkMarker_AsNeedToUpdateTransform(
        FCk_Handle InMarkerHandle)
    -> void
{
    InMarkerHandle.Add<ck::FTag_Marker_UpdateTransform>();
}

// --------------------------------------------------------------------------------------------------------------------
