#include "CkMarker_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
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
        const FCk_Fragment_Marker_ParamsData& InParams)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle), TEXT("Cannot Add a Marker to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return; }

    const auto& owningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle).Get_Actor().Get();
    const auto& markerName = InParams.Get_MarkerName();
    const auto& markerReplicationType = InParams.Get_ReplicationType();
    const auto& outermostRemoteAuthority = [&]()
    {
        if (owningActor->GetIsReplicated())
        { return owningActor; }

        return UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(owningActor);
    }();

    if (NOT UCk_Utils_Net_UE::Get_IsRoleMatching(outermostRemoteAuthority, markerReplicationType))
    {
        ck::overlap_body::VeryVerbose
        (
            TEXT("Skipping creation of Marker [{}] on Actor [{}] because it's Replication Type [{}] does NOT match the Outermost Remote Autority [{}]"),
            markerName,
            owningActor,
            markerReplicationType,
            outermostRemoteAuthority
        );

        return;
    }

    auto markerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    auto markerParams = InParams;
    markerParams.Set_EntityAttachedTo(InHandle);

    markerEntity.Add<ck::FFragment_Marker_Params>(markerParams);
    markerEntity.Add<ck::FFragment_Marker_Current>(markerParams.Get_StartingState());
    markerEntity.Add<ck::FTag_Marker_Setup>();

    UCk_Utils_GameplayLabel_UE::Add(markerEntity, markerName);

    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfMarkers_Utils::AddIfMissing(InHandle);

    RecordOfMarkers_Utils::Request_Connect(InHandle, markerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Has(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> bool
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return ck::IsValid(MarkerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Ensure(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle, InMarkerName),
        TEXT("Handle [{}] does NOT have a Marker with Name [{}]"), InHandle, InMarkerName)
    { return false; }

    return true;
}

auto
    UCk_Utils_Marker_UE::
    PreviewAllMarkers(
        UObject*   InOuter,
        FCk_Handle InHandle)
    -> void
{
    if (NOT RecordOfMarkers_Utils::Has(InHandle))
    { return; }

    RecordOfMarkers_Utils::ForEachEntry(InHandle, [&](FCk_Handle InMarkerEntity)
    {
        DoPreviewMarker(InOuter, InMarkerEntity);
    });
}

auto
    UCk_Utils_Marker_UE::
    PreviewMarker(
        UObject*     InOuter,
        FCk_Handle   InHandle,
        FGameplayTag InMarkerName)
    -> void
{
    if (NOT Ensure(InHandle, InMarkerName))
    { return; }

    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

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
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_PhysicsInfo
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_PhysicsParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_ShapeInfo
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_ShapeParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_DebugInfo
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_DebugParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> ECk_EnableDisable
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Marker_UE::
    Get_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> UShapeComponent*
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Current>().Get_Marker().Get();
}

auto
    UCk_Utils_Marker_UE::
    Get_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_EntityOwningActor_BasicDetails
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    return MarkerEntity.Get<ck::FFragment_Marker_Current>().Get_AttachedEntityAndActor();
}

auto
    UCk_Utils_Marker_UE::
    Request_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest)
    -> void
{
    auto MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InHandle, InMarkerName);

    MarkerEntity.AddOrGet<ck::FFragment_Marker_Requests>()._Requests = InRequest;
}

auto
    UCk_Utils_Marker_UE::
    BindTo_OnEnableDisable(
        FCk_Handle                                 InMarkerHandle,
        FGameplayTag                               InMarkerName,
        ECk_Signal_PayloadInFlight                 InBindingPolicy,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate)
    -> void
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerHandle, InMarkerName);

    ck::UUtils_Signal_OnMarkerEnableDisable::Bind(MarkerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Marker_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle                                 InMarkerHandle,
        FGameplayTag                               InMarkerName,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate)
    -> void
{
    const auto& MarkerEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_Marker_UE,
        RecordOfMarkers_Utils>(InMarkerHandle, InMarkerName);

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
