#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkVoiceChat/Codec/CkVoiceChat_Codec.h"
#include "CkVoiceChat/Playback/CkVoiceChatSynth_Component.h"
#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment_Data.h"

#include <Templates/SharedPointer.h>
#include <UObject/StrongObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceTalker_UE;
class ICk_VoiceChat_CaptureSource;
class IVoiceEncoder;
class IVoiceDecoder;
class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_IsTransmitting);
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_IsSpeaking);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoiceTalker_Params = FCk_Fragment_VoiceTalker_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // One encoded frame awaiting send, timestamped on the capture clock so the pacing step can
    // drop stale entries BEFORE the transport queue ever sees them (the Iris ordered-unreliable
    // path queues silently and never drops - freshness is a send-side responsibility).
    struct CKVOICECHAT_API FCk_VoiceChat_OutboundFrame
    {
    public:
        CK_GENERATED_BODY(FCk_VoiceChat_OutboundFrame);

    private:
        uint16 _Seq = 0;
        TArray<uint8> _Encoded;
        double _CaptureTimeSeconds = 0.0;

    public:
        CK_PROPERTY_GET(_Seq);
        CK_PROPERTY_GET(_Encoded);
        CK_PROPERTY_GET(_CaptureTimeSeconds);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_OutboundFrame, _Seq, _Encoded, _CaptureTimeSeconds);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceTalker_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_Current);

    public:
        friend class FProcessor_VoiceTalker_Setup;
        friend class FProcessor_VoiceTalker_HandleRequests;
        friend class FProcessor_VoiceTalker_Capture;
        friend class FProcessor_VoiceTalker_ReceivePlayback;
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

        TArray<FCk_VoiceChat_OutboundFrame> _OutboundFrames;

        // The playout chain (jitter -> decoder -> decoded PCM -> synth). Shared by the two
        // mutually exclusive users on any one machine: the local self-monitor (loopback, while
        // THIS machine transmits) and remote-talker receive (the entity is a replica; its
        // transmit path never runs here).
        FCk_VoiceChat_JitterBuffer _LoopbackJitter;
        TSharedPtr<IVoiceDecoder> _LoopbackDecoder;
        TArray<uint8> _LoopbackDecodedPcm;
        double _LoopbackPopAccumulatorSeconds = 0.0;
        TStrongObjectPtr<UCk_VoiceChatSynthComponent_UE> _LoopbackSynth;

        double _ReceiveClockSeconds = 0.0;

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

    // Voice is disposable: an inbox nobody drains yet (composition races, a paused world) must
    // never grow unbounded - past this cap new bundles are dropped and counted.
    inline constexpr int32 VoiceChat_MaxInboxBundles = 256;

    // Server-side, on the talker entity: packed bundles that arrived over the relay, stamped with
    // the sending player at the RPC boundary (clients are not trusted to self-identify).
    // Drained by FProcessor_VoiceChat_Route; entries surviving a tick are already stale.
    struct CKVOICECHAT_API FCk_VoiceChat_InboundBundle
    {
    public:
        CK_GENERATED_BODY(FCk_VoiceChat_InboundBundle);

    private:
        TArray<uint8> _PackedBundle;
        TWeakObjectPtr<APlayerState> _Sender;

    public:
        CK_PROPERTY_GET(_PackedBundle);
        CK_PROPERTY_GET(_Sender);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_InboundBundle, _PackedBundle, _Sender);
    };

    struct CKVOICECHAT_API FFragment_VoiceTalker_ServerInbox
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_ServerInbox);

    private:
        TArray<FCk_VoiceChat_InboundBundle> _Bundles;

    public:
        CK_PROPERTY(_Bundles);
    };

    // Client-side, on the talker entity: packed bundles forwarded by the server, awaiting the
    // jitter/decode playback processor.
    struct CKVOICECHAT_API FFragment_VoiceTalker_ReceiveInbox
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_ReceiveInbox);

    private:
        TArray<TArray<uint8>> _PackedBundles;

    public:
        CK_PROPERTY(_PackedBundles);
    };

    // World-scoped (transient entity), authority-side: per-connection bytes forwarded THIS tick.
    // The routing processor iterates per talker, so the cross-talker budget lives here; the map
    // self-resets on the first touch of each new frame.
    struct CKVOICECHAT_API FFragment_VoiceChat_RouteBudgets
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_RouteBudgets);

    private:
        uint64 _LastResetFrame = 0;
        TMap<TWeakObjectPtr<APlayerState>, int32> _SpentBytes;

    public:
        CK_PROPERTY(_LastResetFrame);
        CK_PROPERTY(_SpentBytes);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_TransmitStarted, FCk_Delegate_VoiceTalker, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_TransmitStopped, FCk_Delegate_VoiceTalker, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_SpeakingStateChanged, FCk_Delegate_VoiceTalker_SpeakingStateChanged, FCk_Handle_VoiceTalker, bool);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_FramesCaptured, FCk_Delegate_VoiceTalker_FramesCaptured, FCk_Handle_VoiceTalker, TArray<uint8>, int32);
}

// --------------------------------------------------------------------------------------------------------------------
