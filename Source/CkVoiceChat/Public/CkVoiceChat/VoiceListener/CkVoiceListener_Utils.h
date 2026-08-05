#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"
#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment_Data.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include "CkVoiceListener_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoiceListener"))
class CKVOICECHAT_API UCk_Utils_VoiceListener_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoiceListener_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoiceListener);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Add Feature")
    static FCk_Handle_VoiceListener
    Add(
        UPARAM(ref) FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VoiceChat|Listener",
        DisplayName="[Ck][VoiceListener] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VoiceListener
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VoiceChat|Listener",
        DisplayName="[Ck][VoiceListener] Handle -> VoiceListener Handle",
        meta = (CompactNodeTitle = "<AsVoiceListener>", BlueprintAutocast))
    static FCk_Handle_VoiceListener
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VoiceListener Handle",
        Category = "Ck|Utils|VoiceChat|Listener",
        meta = (CompactNodeTitle = "INVALID_VoiceListenerHandle", Keywords = "make"))
    static FCk_Handle_VoiceListener
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Request Mute Talker",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceListener
    Request_MuteTalker(
        UPARAM(ref) FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Request_VoiceListener_MuteTalker& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Request Unmute Talker",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceListener
    Request_UnmuteTalker(
        UPARAM(ref) FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Request_VoiceListener_UnmuteTalker& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Request Set Talker Volume",
              meta = (AutoCreateRefTerm = "InRequest,InDelegate"))
    static FCk_Handle_VoiceListener
    Request_SetTalkerVolume(
        UPARAM(ref) FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Request_VoiceListener_SetTalkerVolume& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Get Is Talker Muted")
    static bool
    Get_IsTalkerMuted(
        const FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Listener",
              DisplayName="[Ck][VoiceListener] Get Talker Volume")
    static float
    Get_TalkerVolume(
        const FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Handle_VoiceTalker& InVoiceTalker);
};

// --------------------------------------------------------------------------------------------------------------------
