#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include "CkVoiceTalker_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VoiceTalker"))
class CKVOICECHAT_API UCk_Utils_VoiceTalker_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoiceTalker_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VoiceTalker);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Add Feature")
    static FCk_Handle_VoiceTalker
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VoiceTalker_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VoiceChat|Talker",
        DisplayName="[Ck][VoiceTalker] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VoiceTalker
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VoiceChat|Talker",
        DisplayName="[Ck][VoiceTalker] Handle -> VoiceTalker Handle",
        meta = (CompactNodeTitle = "<AsVoiceTalker>", BlueprintAutocast))
    static FCk_Handle_VoiceTalker
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VoiceTalker Handle",
        Category = "Ck|Utils|VoiceChat|Talker",
        meta = (CompactNodeTitle = "INVALID_VoiceTalkerHandle", Keywords = "make"))
    static FCk_Handle_VoiceTalker
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Transmit Mode")
    static ECk_VoiceChat_TransmitMode
    Get_TransmitMode(
        const FCk_Handle_VoiceTalker& InVoiceTalker);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Talker",
              DisplayName="[Ck][VoiceTalker] Get Input Gain")
    static float
    Get_InputGain(
        const FCk_Handle_VoiceTalker& InVoiceTalker);
};

// --------------------------------------------------------------------------------------------------------------------
