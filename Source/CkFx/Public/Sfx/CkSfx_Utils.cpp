#include "CkSfx_Utils.h"

#include "Sfx/CkSfx_Fragment.h"

#include "CkFx/CkFx_Log.h"

#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sfx_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Sfx_ParamsData& InParams)
    -> FCk_Handle_Sfx
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_Sfx_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Sfx_Current>();
    });

    auto NewSfxEntity = Cast(NewEntity);

    RecordOfSfx_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfSfx_Utils::Request_Connect(InHandle, NewSfxEntity);

    return NewSfxEntity;
}

auto
    UCk_Utils_Sfx_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleSfx_ParamsData& InParams)
    -> TArray<FCk_Handle_Sfx>
{
    return ck::algo::Transform<TArray<FCk_Handle_Sfx>>(InParams.Get_SfxParams(),
    [&](const FCk_Fragment_Sfx_ParamsData& InSfxParams)
    {
        return Add(InHandle, InSfxParams);
    });
}

auto
    UCk_Utils_Sfx_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfSfx_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Sfx_UE, FCk_Handle_Sfx, ck::FFragment_Sfx_Current, ck::FFragment_Sfx_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sfx_UE::
    TryGet_Sfx(
        const FCk_Handle& InSfxOwnerEntity,
        FGameplayTag InSfxName)
    -> FCk_Handle_Sfx
{
    return RecordOfSfx_Utils::Get_ValidEntry_If(InSfxOwnerEntity, ck::algo::MatchesGameplayLabelExact{InSfxName});
}

auto
    UCk_Utils_Sfx_UE::
    Get_SoundCue(
        const FCk_Handle_Sfx& InSfxHandle)
    -> USoundBase*
{
    return InSfxHandle.Get<ck::FFragment_Sfx_Params>().Get_Params().Get_SoundCue();
}

auto
    UCk_Utils_Sfx_UE::
    ForEach_Sfx(
        FCk_Handle& InSfxOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Sfx>
{
    auto Sfx = TArray<FCk_Handle_Sfx>{};

    ForEach_Sfx(InSfxOwnerEntity, [&](FCk_Handle_Sfx InSfx)
    {
        Sfx.Emplace(InSfx);

        std::ignore = InDelegate.ExecuteIfBound(InSfx, InOptionalPayload);
    });

    return Sfx;
}

auto
    UCk_Utils_Sfx_UE::
    ForEach_Sfx(
        FCk_Handle& InSfxOwnerEntity,
        const TFunction<void(FCk_Handle_Sfx)>& InFunc)
    -> void
{
    RecordOfSfx_Utils::ForEach_ValidEntry(InSfxOwnerEntity, InFunc);
}

auto
    UCk_Utils_Sfx_UE::
    Request_PlayAttached(
        FCk_Handle_Sfx& InSfxHandle,
        const FCk_Request_Sfx_PlayAttached& InRequest)
    -> FCk_Handle_Sfx
{
    // TODO: Move this to processor

    const auto& SoundCue = Get_SoundCue(InSfxHandle);
    const auto& Params = InSfxHandle.Get<ck::FFragment_Sfx_Params>().Get_Params();

    const auto& AudioSettings = InRequest.Get_OverrideAudioSettings()
                                    ? InRequest.Get_OverridenAudioSettings()
                                    : Params.Get_DefaultAudioSettings();

    constexpr auto StartTime = 0.0f;
    UGameplayStatics::SpawnSoundAttached
    (
        SoundCue,
        InRequest.Get_AttachComponent().Get(),
        NAME_None,
        InRequest.Get_RelativeTransformSettings().Get_Location(),
        InRequest.Get_RelativeTransformSettings().Get_Rotation(),
        EAttachLocation::Type::KeepRelativeOffset,
        InRequest.Get_StopPolicy() == ECk_Sfx_Stop_Policy::StopWhenFinishedOrDetached,
        AudioSettings.Get_VolumeMultipler(),
        AudioSettings.Get_PitchMultipler(),
        StartTime,
        Params.Get_AttenuationSettings(),
        Params.Get_ConcurrencySettings()
    );

    return InSfxHandle;
}

auto
    UCk_Utils_Sfx_UE::
    Request_PlayAtLocation(
        FCk_Handle_Sfx& InSfxHandle,
        const FCk_Request_Sfx_PlayAtLocation& InRequest)
    -> FCk_Handle_Sfx
{
    // TODO: Move this to processor

    const auto& SoundCue = Get_SoundCue(InSfxHandle);
    const auto& Params = InSfxHandle.Get<ck::FFragment_Sfx_Params>().Get_Params();

    const auto& AudioSettings = InRequest.Get_OverrideAudioSettings()
                                    ? InRequest.Get_OverridenAudioSettings()
                                    : Params.Get_DefaultAudioSettings();

    constexpr auto StartTime = 0.0f;
    UGameplayStatics::SpawnSoundAtLocation
    (
        InRequest.Get_Outer().Get(),
        SoundCue,
        InRequest.Get_TransformSettings().Get_Location(),
        InRequest.Get_TransformSettings().Get_Rotation(),
        AudioSettings.Get_VolumeMultipler(),
        AudioSettings.Get_PitchMultipler(),
        StartTime,
        Params.Get_AttenuationSettings(),
        Params.Get_ConcurrencySettings()
    );

    return InSfxHandle;
}

// --------------------------------------------------------------------------------------------------------------------
