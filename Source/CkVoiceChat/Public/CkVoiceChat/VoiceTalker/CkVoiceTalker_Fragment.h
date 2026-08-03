#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkVoiceChat/Codec/CkVoiceChat_Codec.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include <Templates/SharedPointer.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceTalker_UE;
class ICk_VoiceChat_CaptureSource;
class IVoiceEncoder;
class IVoiceDecoder;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_IsTransmitting);
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_IsSpeaking);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoiceTalker_Params = FCk_Fragment_VoiceTalker_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceTalker_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_Current);

    public:
        friend class FProcessor_VoiceTalker_Setup;
        friend class FProcessor_VoiceTalker_HandleRequests;
        friend class FProcessor_VoiceTalker_Capture;
        friend class FProcessor_VoiceTalker_EndPlay;
        friend class UCk_Utils_VoiceTalker_UE;

    private:
        uint16 _Seq = 0;
        uint8 _AmplitudeQ8 = 0;

        // runtime copies of the tunable params (requests mutate these, never the Params fragment)
        ECk_VoiceChat_TransmitMode _TransmitMode = ECk_VoiceChat_TransmitMode::PushToTalk;
        float _InputGain = 1.2f;
        ECk_EnableDisable _SelfMute = ECk_EnableDisable::Disable;

        TSharedPtr<ICk_VoiceChat_CaptureSource> _CaptureSource;
        TSharedPtr<IVoiceEncoder> _Encoder;

        FCk_VoiceChat_Vad _Vad;
        TArray<uint8> _PendingPcm;
        double _CaptureClockSeconds = 0.0;

        // local self-monitor path: encoded frames run back through the real playback policy
        FCk_VoiceChat_JitterBuffer _LoopbackJitter;
        TSharedPtr<IVoiceDecoder> _LoopbackDecoder;
        TArray<uint8> _LoopbackDecodedPcm;
        double _LoopbackPopAccumulatorSeconds = 0.0;

    public:
        CK_PROPERTY_GET(_Seq);
        CK_PROPERTY_GET(_AmplitudeQ8);
        CK_PROPERTY_GET(_TransmitMode);
        CK_PROPERTY_GET(_InputGain);
        CK_PROPERTY_GET(_SelfMute);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceTalker_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_Requests);

    public:
        friend class FProcessor_VoiceTalker_HandleRequests;
        friend class FProcessor_VoiceTalker_CancelPendingRequests;
        friend class UCk_Utils_VoiceTalker_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_VoiceTalker_StartTransmit,
            FCk_Request_VoiceTalker_StopTransmit,
            FCk_Request_VoiceTalker_SetTransmitMode,
            FCk_Request_VoiceTalker_SetInputGain,
            FCk_Request_VoiceTalker_SetSelfMute>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_TransmitStarted, FCk_Delegate_VoiceTalker, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_TransmitStopped, FCk_Delegate_VoiceTalker, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_SpeakingStateChanged, FCk_Delegate_VoiceTalker_SpeakingStateChanged, FCk_Handle_VoiceTalker, bool);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_FramesCaptured, FCk_Delegate_VoiceTalker_FramesCaptured, FCk_Handle_VoiceTalker, TArray<uint8>, int32);
}

// --------------------------------------------------------------------------------------------------------------------
