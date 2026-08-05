#pragma once

#include "CkCore/Macros/CkMacros.h"

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
              Category = "Ck|Utils|VoiceChat",
              DisplayName="[Ck][VoiceListener] Add Feature")
    static FCk_Handle_VoiceListener
    Add(
        UPARAM(ref) FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat",
              DisplayName="[Ck][VoiceListener] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VoiceChat",
        DisplayName="[Ck][VoiceListener] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VoiceListener
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VoiceChat",
        DisplayName="[Ck][VoiceListener] Handle -> VoiceListener Handle",
        meta = (CompactNodeTitle = "<AsVoiceListener>", BlueprintAutocast))
    static FCk_Handle_VoiceListener
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VoiceListener Handle",
        Category = "Ck|Utils|VoiceChat",
        meta = (CompactNodeTitle = "INVALID_VoiceListenerHandle", Keywords = "make"))
    static FCk_Handle_VoiceListener
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat",
              DisplayName="[Ck][VoiceListener] Get Is Talker Muted")
    static bool
    Get_IsTalkerMuted(
        const FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat",
              DisplayName="[Ck][VoiceListener] Get Talker Volume")
    static float
    Get_TalkerVolume(
        const FCk_Handle_VoiceListener& InVoiceListener,
        const FCk_Handle_VoiceTalker& InVoiceTalker);
};

// --------------------------------------------------------------------------------------------------------------------
