#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkVoiceChat/Net/CkVoiceChat_RepData.h"
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
        const FCk_VoiceChannel_Spec& InParams);

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

    // Resolved at channel Setup through CkResourceLoader (nullptr while the async load is
    // pending, if the channel authored none, or if resolution failed - callers fall back to
    // UCk_Utils_VoiceChat_Settings_UE::Get_DefaultAttenuation()).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Resolved Attenuation")
    static USoundAttenuation*
    Get_ResolvedAttenuation(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Resolved Source Effect Chain")
    static USoundEffectSourcePresetChain*
    Get_ResolvedSourceEffectChain(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

public:
    // Membership is server-authoritative: every request below rejects non-authority callers
    // synchronously (Failed_NotEnqueued) - clients receive membership via the control plane.

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Request Join",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceChannel
    Request_Join(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_Join& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Request Leave",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceChannel
    Request_Leave(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_Leave& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Request Set Member Flags",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceChannel
    Request_SetMemberFlags(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_SetMemberFlags& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Request Server Mute",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceChannel
    Request_ServerMute(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_ServerMute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Request Server Unmute",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceChannel
    Request_ServerUnmute(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Request_VoiceChannel_ServerUnmute& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Channel Idx")
    static uint8
    Get_ChannelIdx(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Is Member")
    static bool
    Get_IsMember(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Handle_VoiceTalker& InTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Member Flags")
    static FCk_VoiceChat_MemberFlags
    Get_MemberFlags(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Handle_VoiceTalker& InTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Members")
    static TArray<FCk_Handle_VoiceTalker>
    Get_Members(
        const FCk_Handle_VoiceChannel& InVoiceChannel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Get Is Server Muted")
    static bool
    Get_IsServerMuted(
        const FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Handle_VoiceTalker& InTalker);

    // Resolves a wire ChannelIdx against the world registry. Quiet on miss by design: a stale
    // or forged index is expected traffic (N1) - the caller drops and counts.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Try Get Channel By Idx")
    static FCk_Handle_VoiceChannel
    TryGet_ChannelByIdx(
        const FCk_Handle& InAnyHandle,
        uint8 InChannelIdx);

public:
    // Client-side control-plane applier, called by the registered net handler (see
    // Net/CkVoiceChat_Replication.cpp). NotReady until EVERY named channel is composed on this
    // world (symmetric composition), and mutates nothing before that check passes. Not a
    // UFUNCTION - net plumbing, not consumer API.
    static auto
    Apply_ReplicatedControlPlane(
        FCk_Handle& InChannelHost,
        const FCk_RepData_VoiceChat& InRepData) -> ECk_Persistence_ApplyResult;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Bind To OnMemberJoined")
    static FCk_Handle_VoiceChannel
    BindTo_OnMemberJoined(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Unbind From OnMemberJoined")
    static FCk_Handle_VoiceChannel
    UnbindFrom_OnMemberJoined(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Bind To OnMemberLeft")
    static FCk_Handle_VoiceChannel
    BindTo_OnMemberLeft(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Channel",
              DisplayName="[Ck][VoiceChannel] Unbind From OnMemberLeft")
    static FCk_Handle_VoiceChannel
    UnbindFrom_OnMemberLeft(
        UPARAM(ref) FCk_Handle_VoiceChannel& InVoiceChannel,
        const FCk_Delegate_VoiceChannel_Membership& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
