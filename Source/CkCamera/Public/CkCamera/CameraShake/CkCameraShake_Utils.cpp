#include "CkCameraShake_Utils.h"

#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CameraShake_UE::
    Add(
        FCk_Handle_UnderConstruction& InHandle,
        const FCk_Fragment_CameraShake_ParamsData& InParams)
    -> void
{
    auto NewCameraShakeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InCameraShakeEntity)
    {
        InCameraShakeEntity.Add<ck::FFragment_CameraShake_Params>(InParams);

        UCk_Utils_GameplayLabel_UE::Add(InCameraShakeEntity, InParams.Get_ID());
    });

    RecordOfCameraShakes_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfCameraShakes_Utils::Request_Connect(InHandle, NewCameraShakeEntity);
}

auto
    UCk_Utils_CameraShake_UE::
    AddMultiple(
        FCk_Handle_UnderConstruction& InHandle,
        const FCk_Fragment_MultipleCameraShake_ParamsData& InParams)
    -> void
{
    for (const auto& params : InParams.Get_CameraShakeParams())
    {
        Add(InHandle, params);
    }
}

auto
    UCk_Utils_CameraShake_UE::
    Has(
        FCk_Handle InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName)
    -> bool
{
    const auto& CameraShakeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_CameraShake_UE,
        RecordOfCameraShakes_Utils>(InCameraShakeOwnerEntity, InCameraShakeName);

    return ck::IsValid(CameraShakeEntity);
}

auto
    UCk_Utils_CameraShake_UE::
    Has_Any(
        FCk_Handle InCameraShakeOwnerEntity)
    -> bool
{
    return RecordOfCameraShakes_Utils::Has(InCameraShakeOwnerEntity);
}

auto
    UCk_Utils_CameraShake_UE::
    Ensure(
        FCk_Handle InCameraShakeOwnerEntity,
        FGameplayTag InCameraShakeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InCameraShakeOwnerEntity, InCameraShakeName), TEXT("Handle [{}] does NOT have CameraShake [{}]"), InCameraShakeOwnerEntity, InCameraShakeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_CameraShake_UE::
    Ensure_Any(
        FCk_Handle InCameraShakeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InCameraShakeOwnerEntity), TEXT("Handle [{}] does NOT have any CameraShake [{}]"), InCameraShakeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_CameraShake_UE::
    Request_PlayOnTarget(
        FCk_Handle InCameraShakeOwnerEntity,
        const FCk_Request_CameraShake_PlayOnTarget& InRequest)
    -> void
{
    if (NOT Ensure(InCameraShakeOwnerEntity, InRequest.Get_CameraShakeID()))
    { return; }

    auto CameraShakeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_CameraShake_UE,
        RecordOfCameraShakes_Utils>(InCameraShakeOwnerEntity, InRequest.Get_CameraShakeID());

    CameraShakeEntity.AddOrGet<ck::FFragment_CameraShake_Requests>()._PlayRequests.Emplace(InRequest);
}

auto
    UCk_Utils_CameraShake_UE::
    Request_PlayAtLocation(
        FCk_Handle InCameraShakeOwnerEntity,
        const FCk_Request_CameraShake_PlayAtLocation& InRequest)
    -> void
{
    if (NOT Ensure(InCameraShakeOwnerEntity, InRequest.Get_CameraShakeID()))
    { return; }

    auto CameraShakeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_CameraShake_UE,
        RecordOfCameraShakes_Utils>(InCameraShakeOwnerEntity, InRequest.Get_CameraShakeID());

    CameraShakeEntity.AddOrGet<ck::FFragment_CameraShake_Requests>()._PlayRequests.Emplace(InRequest);
}

auto
    UCk_Utils_CameraShake_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_CameraShake_Params>();
}

// --------------------------------------------------------------------------------------------------------------------
