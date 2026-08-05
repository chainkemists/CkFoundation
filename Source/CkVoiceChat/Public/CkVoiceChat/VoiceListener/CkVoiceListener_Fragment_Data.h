#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include "CkVoiceListener_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVOICECHAT_API FCk_Handle_VoiceListener : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VoiceListener); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VoiceListener);

// --------------------------------------------------------------------------------------------------------------------

// Client mute is a PRIVACY property, not a playback preference: the mute set reports upstream
// as a routing exclusion, so the server stops forwarding this talker's audio to this player -
// muted audio never reaches the muting machine at all.
USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceListener_MuteTalker : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceListener_MuteTalker);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceListener_MuteTalker);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

public:
    CK_PROPERTY_GET(_Talker);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceListener_MuteTalker, _Talker);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceListener_UnmuteTalker : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceListener_UnmuteTalker);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceListener_UnmuteTalker);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

public:
    CK_PROPERTY_GET(_Talker);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceListener_UnmuteTalker, _Talker);
};

// --------------------------------------------------------------------------------------------------------------------

// Volume is LOCAL-only (applied at playback on this machine) - it never travels upstream.
USTRUCT(BlueprintType)
struct CKVOICECHAT_API FCk_Request_VoiceListener_SetTalkerVolume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VoiceListener_SetTalkerVolume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VoiceListener_SetTalkerVolume);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VoiceTalker _Talker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _Volume = 1.0f;

public:
    CK_PROPERTY_GET(_Talker);
    CK_PROPERTY_GET(_Volume);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VoiceListener_SetTalkerVolume, _Talker, _Volume);
};

// --------------------------------------------------------------------------------------------------------------------
