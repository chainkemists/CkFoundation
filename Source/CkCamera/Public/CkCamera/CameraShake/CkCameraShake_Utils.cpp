#include "CkCameraShake_Utils.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraShake_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_CameraShake_ParamsData& InParams)
    -> FCk_Handle_CameraShake
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_CameraShake_Params>(InParams);
    });

    auto NewCameraShakeEntity = Cast(NewEntity);

    RecordOfCameraShakes_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfCameraShakes_Utils::Request_Connect(InHandle, NewCameraShakeEntity);

    return NewCameraShakeEntity;
}

auto
    UCk_Utils_CameraShake_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleCameraShake_ParamsData& InParams)
    -> TArray<FCk_Handle_CameraShake>
{
    return ck::algo::Transform<TArray<FCk_Handle_CameraShake>>(InParams.Get_CameraShakeParams(),
    [&](const FCk_Fragment_CameraShake_ParamsData& InCameraShakeParams)
    {
        return Add(InHandle, InCameraShakeParams);
    });
}

auto
    UCk_Utils_CameraShake_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfCameraShakes_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CameraShake_UE, FCk_Handle_CameraShake, ck::FFragment_CameraShake_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraShake_UE::
    TryGet_CameraShake(
        const FCk_Handle& InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName)
    -> FCk_Handle_CameraShake
{
    return RecordOfCameraShakes_Utils::Get_ValidEntry_If(InCameraShakeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InCameraShakeName});
}

auto
    UCk_Utils_CameraShake_UE::
    ForEach_CameraShake(
        FCk_Handle& InCameraShakeOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_CameraShake>
{
    auto CameraShakes = TArray<FCk_Handle_CameraShake>{};

    ForEach_CameraShake(InCameraShakeOwnerEntity, [&](FCk_Handle_CameraShake InCameraShake)
    {
        CameraShakes.Emplace(InCameraShake);

        std::ignore = InDelegate.ExecuteIfBound(InCameraShake, InOptionalPayload);
    });

    return CameraShakes;
}

auto
    UCk_Utils_CameraShake_UE::
    ForEach_CameraShake(
        FCk_Handle& InCameraShakeOwnerEntity,
        const TFunction<void(FCk_Handle_CameraShake)>& InFunc)
    -> void
{
    RecordOfCameraShakes_Utils::ForEach_ValidEntry(InCameraShakeOwnerEntity, InFunc);
}

auto
    UCk_Utils_CameraShake_UE::
    Request_PlayOnTarget(
        UPARAM(ref) FCk_Handle_CameraShake& InCameraShakeHandle,
        const FCk_Request_CameraShake_PlayOnTarget& InRequest)
    -> void
{
    InCameraShakeHandle.AddOrGet<ck::FFragment_CameraShake_Requests>()._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_CameraShake_UE::
    Request_PlayAtLocation(
        UPARAM(ref) FCk_Handle_CameraShake& InCameraShakeHandle,
        const FCk_Request_CameraShake_PlayAtLocation& InRequest)
    -> void
{
    InCameraShakeHandle.AddOrGet<ck::FFragment_CameraShake_Requests>()._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------
