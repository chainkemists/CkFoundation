#include "CkMarker_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
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
    const auto& outermostRemoteAuthority = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(owningActor);

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

    markerEntity.AddOrGet<ck::FCk_Fragment_Marker_Params>(markerParams);
    markerEntity.AddOrGet<ck::FCk_Fragment_Marker_Current>(markerParams.Get_StartingState());
    markerEntity.Add<ck::FCk_Tag_Marker_Setup>();

    UCk_Utils_GameplayLabel_UE::Add(markerEntity, markerName);

    if (NOT RecordOfMarkers_Utils::Has(InHandle))
    {
        // TODO: Select Record policy that disallow duplicate based on Gameplay Label
        RecordOfMarkers_Utils::Add(InHandle);
    }

    RecordOfMarkers_Utils::Request_Connect(InHandle, markerEntity);
}

auto
    UCk_Utils_Marker_UE::
    DoHas(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> bool
{
    return Has(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoEnsure(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> bool
{
    return Ensure(InHandle, InMarkerName);
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
        const auto& markerName = InMarkerEntity.Get<ck::FCk_Fragment_Marker_Params>().Get_Params().Get_MarkerName();
        PreviewSingleMarker<ECk_FragmentQuery_Policy::CurrentEntity>(InOuter, InMarkerEntity, markerName);
    });
}

auto
    UCk_Utils_Marker_UE::
    DoGet_PhysicsInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_PhysicsInfo
{
    return Get_PhysicsInfo(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoGet_ShapeInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_ShapeInfo
{
    return Get_ShapeInfo(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoGet_DebugInfo(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Marker_DebugInfo
{
    return Get_DebugInfo(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoGet_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> ECk_EnableDisable
{
    return Get_EnableDisable(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoGet_ShapeComponent(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> UShapeComponent*
{
    return Get_ShapeComponent(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoGet_AttachedEntityAndActor(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName)
    -> FCk_EntityOwningActor_BasicDetails
{
    return Get_AttachedEntityAndActor(InHandle, InMarkerName);
}

auto
    UCk_Utils_Marker_UE::
    DoRequest_EnableDisable(
        FCk_Handle InHandle,
        FGameplayTag InMarkerName,
        const FCk_Request_Marker_EnableDisable& InRequest)
    -> void
{
    Request_EnableDisable(InHandle, InMarkerName, InRequest);
}

auto
    UCk_Utils_Marker_UE::
    Request_MarkMarker_AsNeedToUpdateTransform(
        FCk_Handle InMarkerHandle)
    -> void
{
    InMarkerHandle.Add<ck::FCk_Tag_Marker_UpdateTransform>();
}

// --------------------------------------------------------------------------------------------------------------------
