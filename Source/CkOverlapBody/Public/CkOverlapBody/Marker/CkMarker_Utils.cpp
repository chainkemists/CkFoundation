#include "CkMarker_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkNet/CkNet_Utils.h"
#include "CkOverlapBody/CkOverlapBody_Log.h"
#include "CkOverlapBody/Marker/CkMarker_Fragment.h"
#include "CkOverlapBody/MarkerAndSensor/CkMarkerAndSensor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Marker_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Marker_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> FCk_Handle_Marker
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Cannot Add a Marker to Entity [{}] because it does NOT have an Owning Actor"), InHandle)
    { return {}; }

    const auto& MarkerName = InParams.Get_MarkerName();

    if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InHandle, InReplicationType))
    {
        ck::overlap_body::VeryVerbose
        (
            TEXT("Skipping creation of Marker [{}] on Entity [{}] because it's Replication Type [{}] does NOT match"),
            MarkerName,
            InHandle,
            InReplicationType
        );

        return {};
    }

    auto ParamsToUse = InParams;
    ParamsToUse.Set_ReplicationType(InReplicationType);

    auto NewMarkerEntity = CastChecked(UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle,
    [&](FCk_Handle InMarkerEntity)
    {
        InMarkerEntity.Add<ck::FFragment_Marker_Params>(ParamsToUse);
        InMarkerEntity.Add<ck::FFragment_Marker_Current>(ParamsToUse.Get_StartingState());
        InMarkerEntity.Add<ck::FTag_Marker_NeedsSetup>();

        UCk_Utils_GameplayLabel_UE::Add(InMarkerEntity, MarkerName);
    }));

    RecordOfMarkers_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfMarkers_Utils::Request_Connect(InHandle, NewMarkerEntity);

    return NewMarkerEntity;
}

auto
    UCk_Utils_Marker_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleMarker_ParamsData& InParams,
        ECk_Net_ReplicationType InReplicationType)
    -> TArray<FCk_Handle_Marker>
{
    return ck::algo::Transform<TArray<FCk_Handle_Marker>>(InParams.Get_MarkerParams(),
    [&](const FCk_Fragment_Marker_ParamsData& InMarkerParams)
    {
        return Add(InHandle, InMarkerParams, InReplicationType);
    });
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Marker, UCk_Utils_Marker_UE, FCk_Handle_Marker, ck::FFragment_Marker_Params, ck::FFragment_Marker_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Marker_UE::
    TryGet_Marker(
        const FCk_Handle& InMarkerOwnerEntity,
        FGameplayTag      InMarkerName)
    -> FCk_Handle_Marker
{
    return RecordOfMarkers_Utils::Get_ValidEntry_If(InMarkerOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InMarkerName});
}

auto
    UCk_Utils_Marker_UE::
    TryGet_Entity_WithMarker_InOwnershipChain(
        const FCk_Handle& InHandle,
        FGameplayTag InMarkerName)
    -> FCk_Handle
{
    auto MaybeMarkerOwner = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& Handle)
    {
        if (ck::IsValid(TryGet_Marker(Handle, InMarkerName)))
        { return true; }

        return false;
    });

    return MaybeMarkerOwner;
}

auto
    UCk_Utils_Marker_UE::
    Get_Name(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InMarkerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Get_All(
        FCk_Handle InMarkerOwnerEntity)
    -> TArray<FCk_Handle_Marker>
{
    if (NOT Has_Any(InMarkerOwnerEntity))
    { return {}; }

    auto AllMarkers = TArray<FCk_Handle_Marker>{};

    RecordOfMarkers_Utils::ForEach_ValidEntry(InMarkerOwnerEntity, [&](FCk_Handle_Marker InMarkerEntity)
    {
        AllMarkers.Emplace(InMarkerEntity);
    });

    return AllMarkers;
}

auto
    UCk_Utils_Marker_UE::
    PreviewAll(
        UObject* InOuter,
        const FCk_Handle& InHandle)
    -> void
{
    if (NOT Has_Any(InHandle))
    { return; }

    RecordOfMarkers_Utils::ForEach_ValidEntry(InHandle, [&](const FCk_Handle_Marker& InMarkerEntity)
    {
        DoPreviewMarker(InOuter, InMarkerEntity);
    });
}

auto
    UCk_Utils_Marker_UE::
    Preview(
        UObject* InOuter,
        const FCk_Handle_Marker& InMarkerEntity)
    -> void
{
    DoPreviewMarker(InOuter, InMarkerEntity);
}

auto
    UCk_Utils_Marker_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfMarkers_Utils::Has(InHandle);
}

auto
    UCk_Utils_Marker_UE::
    Get_PhysicsInfo(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FCk_Marker_PhysicsInfo
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_PhysicsParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_ShapeInfo(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FCk_Marker_ShapeInfo
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_ShapeParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_DebugInfo(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FCk_Marker_DebugInfo
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_DebugParams();
}

auto
    UCk_Utils_Marker_UE::
    Get_ReplicationType(
        const FCk_Handle_Marker& InMarkerEntity)
    -> ECk_Net_ReplicationType
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_ReplicationType();
}

auto
    UCk_Utils_Marker_UE::
    Get_RelativeTransform(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FTransform
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_RelativeTransform();
}

auto
    UCk_Utils_Marker_UE::
    Get_RelativeLocation(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FVector
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_RelativeTransform().GetLocation();
}

auto
    UCk_Utils_Marker_UE::
    Get_RelativeRotation(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FRotator
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_RelativeTransform().GetRotation().Rotator();
}

auto
    UCk_Utils_Marker_UE::
    Get_RelativeScale(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FVector
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Params>().Get_Params().Get_RelativeTransform().GetScale3D();
}

auto
    UCk_Utils_Marker_UE::
    Get_WorldTransform(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FTransform
{
    auto Shape = Get_ShapeComponent(InMarkerEntity);

    CK_ENSURE_IF_NOT(ck::IsValid(Shape),
        TEXT("Unable to Get_WorldTransform of Marker. Marker Entity [{}] Shape [{}] is NOT valid."), InMarkerEntity, Shape)
    { return {}; }

    return Shape->GetComponentTransform();
}

auto
    UCk_Utils_Marker_UE::
    Get_WorldLocation(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FVector
{
    return Get_WorldTransform(InMarkerEntity).GetLocation();
}

auto
    UCk_Utils_Marker_UE::
    Get_WorldRotation(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FRotator
{
    return Get_WorldTransform(InMarkerEntity).Rotator();
}

auto
    UCk_Utils_Marker_UE::
    Get_EnableDisable(
        const FCk_Handle_Marker& InMarkerEntity)
    -> ECk_EnableDisable
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Current>().Get_EnableDisable();
}

auto
    UCk_Utils_Marker_UE::
    Get_ShapeComponent(
        const FCk_Handle_Marker& InMarkerEntity)
    -> UShapeComponent*
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Current>().Get_Marker().Get();
}

auto
    UCk_Utils_Marker_UE::
    Get_AttachedEntityAndActor(
        const FCk_Handle_Marker& InMarkerEntity)
    -> FCk_EntityOwningActor_BasicDetails
{
    return InMarkerEntity.Get<ck::FFragment_Marker_Current>().Get_AttachedEntityAndActor();
}

auto
    UCk_Utils_Marker_UE::
    Request_EnableDisable(
        FCk_Handle_Marker& InMarkerEntity,
        const FCk_Request_Marker_EnableDisable& InRequest)
    -> FCk_Handle_Marker
{
    InMarkerEntity.AddOrGet<ck::FFragment_Marker_Requests>()._EnableDisableRequest = InRequest;
    return InMarkerEntity;
}

auto
    UCk_Utils_Marker_UE::
    Request_Resize(
        FCk_Handle_Marker& InMarkerEntity,
        const FCk_Request_Marker_Resize& InRequest)
    -> FCk_Handle_Marker
{
    InMarkerEntity.AddOrGet<ck::FFragment_Marker_Requests>()._ResizeRequest = InRequest;

    return InMarkerEntity;
}

auto
    UCk_Utils_Marker_UE::
    BindTo_OnEnableDisable(
        FCk_Handle_Marker& InMarkerEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnMarkerEnableDisable::Bind(InMarkerEntity, InDelegate, InBindingPolicy);
}

auto
    UCk_Utils_Marker_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle_Marker& InMarkerEntity,
        const FCk_Delegate_Marker_OnEnableDisable& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnMarkerEnableDisable::Unbind(InMarkerEntity, InDelegate);
}

auto
    UCk_Utils_Marker_UE::
    DoPreviewMarker(
        UObject* InOuter,
        const FCk_Handle_Marker& InHandle)
    -> void
{
    const auto& Params = InHandle.Get<ck::FFragment_Marker_Params>();
    const auto& Current = InHandle.Get<ck::FFragment_Marker_Current>();

    UCk_Utils_MarkerAndSensor_UE::Draw_Marker_DebugLines(InOuter, Current, Params.Get_Params());
}

auto
    UCk_Utils_Marker_UE::
    Request_MarkMarker_AsNeedToUpdateTransform(
        FCk_Handle_Marker& InMarkerEntity)
    -> void
{
    InMarkerEntity.Add<ck::FTag_Marker_UpdateTransform>();
}

// --------------------------------------------------------------------------------------------------------------------
