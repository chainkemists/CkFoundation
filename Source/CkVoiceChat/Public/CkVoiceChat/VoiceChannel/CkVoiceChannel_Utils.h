#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment_Data.h"

#include "CkVoiceChannel_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoiceChannel"))
class CKVOICECHAT_API UCk_Utils_VoiceChannel_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoiceChannel_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoiceChannel);

private:
    struct RecordOfVoiceChannels_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVoiceChannels> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Creates the channel as a child entity under the host, labeled by its channel tag, and
    // connects it to the host's record of channels (CkAudio Director->Track topology).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Add New Channel")
    static FCk_Handle_VoiceChannel
    Add(
        UPARAM(ref) FCk_Handle& InChannelHost,
        const FCk_Fragment_VoiceChannel_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Has Any Channel")
    static bool
    Has_Any(
        const FCk_Handle& InChannelHost);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VoiceChat|Channel",
        DisplayName="[Ck][VoiceChannel] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VoiceChannel
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VoiceChat|Channel",
        DisplayName="[Ck][VoiceChannel] Handle -> VoiceChannel Handle",
        meta = (CompactNodeTitle = "<AsVoiceChannel>", BlueprintAutocast))
    static FCk_Handle_VoiceChannel
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VoiceChannel Handle",
        Category = "Ck|Utils|VoiceChat|Channel",
        meta = (CompactNodeTitle = "INVALID_VoiceChannelHandle", Keywords = "make"))
    static FCk_Handle_VoiceChannel
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Try Get Channel")
    static FCk_Handle_VoiceChannel
    TryGet_VoiceChannel(
        const FCk_Handle& InChannelHost,
        UPARAM(meta = (Categories = "VoiceChat.Channel")) FGameplayTag InChannelName);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Channel Name")
    static FGameplayTag
    Get_ChannelName(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Spatialization Policy")
    static ECk_VoiceChat_SpatializationPolicy
    Get_SpatializationPolicy(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Audible Range")
    static float
    Get_AudibleRange(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Priority")
    static int32
    Get_Priority(
        const FCk_Handle_VoiceChannel& InVoiceChannel);
};

// --------------------------------------------------------------------------------------------------------------------
