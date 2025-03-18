#include "CkFeedbackCascade_Utils.h"

#include "CkCamera/CameraShake/CkCameraShake_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "Sfx/CkSfx_Utils.h"
#include "Vfx/CkVfx_Utils.h"

#include "CkFeedback/FeedbackCascade/CkFeedbackCascade_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FeedbackCascade_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_FeedbackCascade_ParamsData& InParams)
    -> FCk_Handle_FeedbackCascade
{
    auto NewFeedbackCascadeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_FeedbackCascade>(InHandle);

    UCk_Utils_GameplayLabel_UE::Add(NewFeedbackCascadeEntity, InParams.Get_Name());
    NewFeedbackCascadeEntity.Add<ck::FFragment_FeedbackCascade_Params>(InParams);

    RecordOfFeedbackCascade_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfFeedbackCascade_Utils::Request_Connect(InHandle, NewFeedbackCascadeEntity);

    if (InParams.Get_HasVfx())
    {
        UCk_Utils_Vfx_UE::AddMultiple(NewFeedbackCascadeEntity, InParams.Get_Vfx());
    }

    if (InParams.Get_HasSfx())
    {
        UCk_Utils_Sfx_UE::AddMultiple(NewFeedbackCascadeEntity, InParams.Get_Sfx());
    }

    if (InParams.Get_HasCameraShake())
    {
        UCk_Utils_CameraShake_UE::AddMultiple(NewFeedbackCascadeEntity, InParams.Get_CameraShake());
    }

    return NewFeedbackCascadeEntity;
}

auto
    UCk_Utils_FeedbackCascade_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleFeedbackCascade_ParamsData& InParams)
    -> TArray<FCk_Handle_FeedbackCascade>
{
    return ck::algo::Transform<TArray<FCk_Handle_FeedbackCascade>>(InParams.Get_FeedbackCascadeParams(),
    [&](const FCk_Fragment_FeedbackCascade_ParamsData& InFeedbackCascadeParams)
    {
        return Add(InHandle, InFeedbackCascadeParams);
    });
}

auto
    UCk_Utils_FeedbackCascade_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfFeedbackCascade_Utils::Has(InHandle);
}

auto
    UCk_Utils_FeedbackCascade_UE::
    TryGet_FeedbackCascade(
        const FCk_Handle& InFeedbackCascadeOwnerEntity,
        FGameplayTag InFeedbackCascadeName)
    -> FCk_Handle_FeedbackCascade
{
    return RecordOfFeedbackCascade_Utils::Get_ValidEntry_If(InFeedbackCascadeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InFeedbackCascadeName});
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_FeedbackCascade_UE, FCk_Handle_FeedbackCascade, ck::FFragment_FeedbackCascade_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FeedbackCascade_UE::
    Request_PlayAtLocation(
        UObject* InContextObject,
        FCk_Handle_FeedbackCascade& InFeedbackCascadeHandle,
        const FCk_Request_FeedbackCascade_PlayAtLocation& InRequest)
    -> FCk_Handle_FeedbackCascade
{
    const auto VfxTransformSettings = FCk_Vfx_TransformSettings{}
                                        .Set_Location(InRequest.Get_Location())
                                        .Set_Rotation(InRequest.Get_Rotation());

    UCk_Utils_Vfx_UE::ForEach_Vfx(InFeedbackCascadeHandle, [&](FCk_Handle_Vfx InVfx)
    {
        UCk_Utils_Vfx_UE::Request_PlayAtLocation(InVfx, FCk_Request_Vfx_PlayAtLocation{InContextObject, VfxTransformSettings});
    });

    const auto SfxTransformSettings = FCk_Sfx_TransformSettings{}
                                        .Set_Location(InRequest.Get_Location())
                                        .Set_Rotation(InRequest.Get_Rotation());

    UCk_Utils_Sfx_UE::ForEach_Sfx(InFeedbackCascadeHandle, [&](FCk_Handle_Sfx InSfx)
    {
        UCk_Utils_Sfx_UE::Request_PlayAtLocation(InSfx, FCk_Request_Sfx_PlayAtLocation{InContextObject, SfxTransformSettings});
    });

    UCk_Utils_CameraShake_UE::ForEach_CameraShake(InFeedbackCascadeHandle, [&](FCk_Handle_CameraShake InCameraShake)
    {
        UCk_Utils_CameraShake_UE::Request_PlayAtLocation(InCameraShake, FCk_Request_CameraShake_PlayAtLocation{InRequest.Get_Location(), InContextObject});
    });

    // TODO: Add support for AI Noise
    // TODO: Add support for ForceFeedback (controller rumble)

    return InFeedbackCascadeHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FeedbackCascade_UE::
    Request_PlayAttached(
        FCk_Handle_FeedbackCascade& InFeedbackCascadeHandle,
        const FCk_Request_FeedbackCascade_PlayAttached& InRequest)
    -> FCk_Handle_FeedbackCascade
{
    const auto& AttachedComponent = InRequest.Get_AttachComponent().Get();
    CK_ENSURE_IF_NOT(ck::IsValid(AttachedComponent),
        TEXT("Invalid AttachComponent supplied to FeedbackCascade [{}]! Unable to Play Attached!"),
        AttachedComponent)
    { return InFeedbackCascadeHandle; }

    const auto VfxRelativeTransformSettings = FCk_Vfx_TransformSettings{}
                                        .Set_Location(InRequest.Get_AttachedLocation())
                                        .Set_Rotation(InRequest.Get_AttachedRotation());

    UCk_Utils_Vfx_UE::ForEach_Vfx(InFeedbackCascadeHandle, [&](FCk_Handle_Vfx InVfx)
    {
        UCk_Utils_Vfx_UE::Request_PlayAttached(InVfx, FCk_Request_Vfx_PlayAttached{AttachedComponent}
                                                        .Set_RelativeTransformSettings(VfxRelativeTransformSettings));
    });

    const auto SfxRelativeTransformSettings = FCk_Sfx_TransformSettings{}
                                        .Set_Location(InRequest.Get_AttachedLocation())
                                        .Set_Rotation(InRequest.Get_AttachedRotation());

    UCk_Utils_Sfx_UE::ForEach_Sfx(InFeedbackCascadeHandle, [&](FCk_Handle_Sfx InSfx)
    {
        UCk_Utils_Sfx_UE::Request_PlayAttached(InSfx, FCk_Request_Sfx_PlayAttached{AttachedComponent}.
                                                        Set_RelativeTransformSettings(SfxRelativeTransformSettings));
    });

    UCk_Utils_CameraShake_UE::ForEach_CameraShake(InFeedbackCascadeHandle, [&](FCk_Handle_CameraShake InCameraShake)
    {
        UCk_Utils_CameraShake_UE::Request_PlayAtLocation(InCameraShake, FCk_Request_CameraShake_PlayAtLocation{AttachedComponent->GetComponentToWorld().GetLocation(), AttachedComponent});
    });

    // TODO: Add support for AI Noise
    // TODO: Add support for ForceFeedback (controller rumble)

    return InFeedbackCascadeHandle;
}

// --------------------------------------------------------------------------------------------------------------------
