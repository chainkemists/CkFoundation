#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include "CkVoiceChat/Codec/CkVoiceChat_Codec.h"
#include "CkVoiceChat/Playback/CkVoiceChatSynth_Component.h"
#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment_Data.h"
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
    CK_DEFINE_ECS_TAG(FTag_VoiceTalker_ProximityDirty);

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
        FCk_Time _CaptureTime;

    public:
        CK_PROPERTY_GET(_Seq);
        CK_PROPERTY_GET(_Encoded);
        CK_PROPERTY_GET(_CaptureTime);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_OutboundFrame, _Seq, _Encoded, _CaptureTime);
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
        friend class FProcessor_VoiceChat_Route;
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
        FCk_Time _CaptureClock;

        TArray<FCk_VoiceChat_OutboundFrame> _OutboundFrames;

        // Server-side slew-limited loudness (self-reported amplitude is not trusted raw): the
        // fairness input for the audible-speaker cap. Rise-limited so a spoofed instant-max
        // claim cannot jump the queue; falls when the talker goes quiet.
        float _FairnessEnvelope = 0.0f;

        // The playout chain (jitter -> decoder -> decoded PCM -> synth). Shared by the two
        // mutually exclusive users on any one machine: the local self-monitor (loopback, while
        // THIS machine transmits) and remote-talker receive (the entity is a replica; its
        // transmit path never runs here).
        FCk_VoiceChat_JitterBuffer _LoopbackJitter;
        TSharedPtr<IVoiceDecoder> _LoopbackDecoder;
        TArray<uint8> _LoopbackDecodedPcm;
        FCk_Time _LoopbackPopAccumulator;
        TStrongObjectPtr<UCk_VoiceChatSynthComponent_UE> _LoopbackSynth;

        // The channel whose audio config (spatialization/attenuation/effect chain) the synth
        // currently renders with: the highest-Priority channel delivering this talker's stream
        // to this machine. Invalid for capture-loopback (flat playback, no channel context).
        FCk_Handle_VoiceChannel _PlaybackConfigChannel;

        // HybridRadio render mode on this machine: near = spatialized proximity speech, far =
        // flat radio through the channel's effect chain. Unset until the first distance
        // evaluation, and whenever the config channel is not HybridRadio.
        TOptional<bool> _HybridRenderNear;

        // Zero until this machine receives its first forwarded bundle - the guard that keeps the
        // remote amplitude release from ever touching a capture-loopback talker's amplitude.
        FCk_Time _LastBundleArrivalClock;

        FCk_Time _ReceiveClock;

    public:
        CK_PROPERTY_GET(_Seq);
        CK_PROPERTY_GET(_AmplitudeQ8);
        CK_PROPERTY_GET(_TransmitMode);
        CK_PROPERTY_GET(_InputGain);
        CK_PROPERTY_GET(_SelfMute);
        CK_PROPERTY_GET(_PlaybackConfigChannel);
        CK_PROPERTY_GET(_HybridRenderNear);
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

    public:
        friend class FProcessor_VoiceChat_Route;

    private:
        TArray<FCk_VoiceChat_InboundBundle> _Bundles;

        // N2's per-talker attribution: at most one Warning per talker per cooldown window
        // summarizing that drain's drops - the stats carry exact totals, the ensure carries the
        // first diagnostic, this keeps a SECOND offending talker visible without log spam.
        FCk_Time _DropWarnCooldown = FCk_Time{0.0f};

    public:
        CK_PROPERTY(_Bundles);
    };

    // The presence dot other talkers' range probes detect. Sphere-sphere contact begins at
    // range+margin+this, so it only pads the CANDIDATE boundary - audibility is enforced by the
    // exact distance checks in the routing gate, never by contact geometry.
    inline constexpr float VoiceChat_PresenceProbeRadiusCm = 32.0f;

    // Authority-side, on the talker entity: the Positional3D routing set (ADR-6). The range probe
    // (audible range + hysteresis margin) maintains the candidate set event-driven via Jolt
    // contacts; _AudibleOn holds the per-recipient hysteresis state - a recipient enters a
    // channel's audible set at that channel's range and leaves it at range + margin, so speakers
    // at the boundary don't pop mid-word.
    struct CKVOICECHAT_API FFragment_VoiceTalker_Proximity
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_Proximity);

    public:
        friend class FProcessor_VoiceChat_ProximityProbes;
        friend class FProcessor_VoiceChat_Route;
        friend class UCk_Utils_VoiceTalker_UE;

    private:
        FCk_Handle_Probe _RangeProbe;
        FCk_Handle_ShapeSphere _RangeShape;
        FCk_Handle _RangeProbeChild;
        FCk_Handle _PresenceProbeChild;
        float _RangeProbeRadius = 0.0f;
        TMap<FCk_Handle, TSet<uint8>> _AudibleOn;

    public:
        CK_PROPERTY_GET(_RangeProbe);
    };

    // Client-side, on the talker entity: packed bundles forwarded by the server, awaiting the
    // jitter/decode playback processor. The arrival totals count at the RPC boundary - BEFORE
    // the capacity check - so they measure what the connection actually delivered (the S5
    // drain-budget instrumentation), not what the inbox accepted.
    struct CKVOICECHAT_API FFragment_VoiceTalker_ReceiveInbox
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceTalker_ReceiveInbox);

    private:
        TArray<TArray<uint8>> _PackedBundles;
        uint64 _TotalArrivedBundles = 0;
        uint64 _TotalArrivedBytes = 0;

    public:
        CK_PROPERTY(_PackedBundles);
        CK_PROPERTY(_TotalArrivedBundles);
        CK_PROPERTY(_TotalArrivedBytes);
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

    // One authorized forward awaiting the fairness/budget flush. The Route processor stages
    // per talker; the flush processor sees ALL talkers competing for a recipient this tick -
    // the audible-speaker cap needs that cross-talker view.
    struct CKVOICECHAT_API FCk_VoiceChat_PendingForward
    {
    public:
        CK_GENERATED_BODY(FCk_VoiceChat_PendingForward);

    private:
        TWeakObjectPtr<APlayerState> _Recipient;
        FCk_Handle _Talker;
        TArray<uint8> _PackedBundle;
        float _Envelope = 0.0f;

    public:
        CK_PROPERTY_GET(_Recipient);
        CK_PROPERTY_GET(_Talker);
        CK_PROPERTY_GET(_PackedBundle);
        CK_PROPERTY_GET(_Envelope);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_PendingForward, _Recipient, _Talker, _PackedBundle, _Envelope);
    };

    struct CKVOICECHAT_API FFragment_VoiceChat_PendingForwards
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_PendingForwards);

    private:
        TArray<FCk_VoiceChat_PendingForward> _Forwards;

    public:
        CK_PROPERTY(_Forwards);
    };

    // World-scoped (transient entity), authority-side: when each recipient last received each
    // talker - the least-recently-served rotation input at speaker-cap saturation.
    struct CKVOICECHAT_API FFragment_VoiceChat_ServeHistory
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_ServeHistory);

    private:
        TMap<TWeakObjectPtr<APlayerState>, TMap<FCk_Handle, uint64>> _LastServedFrame;

    public:
        CK_PROPERTY(_LastServedFrame);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_TransmitStarted, FCk_Delegate_VoiceTalker, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_TransmitStopped, FCk_Delegate_VoiceTalker, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_SpeakingStateChanged, FCk_Delegate_VoiceTalker_SpeakingStateChanged, FCk_Handle_VoiceTalker, bool);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceTalker_FramesCaptured, FCk_Delegate_VoiceTalker_FramesCaptured, FCk_Handle_VoiceTalker, TArray<uint8>, int32);
}

// --------------------------------------------------------------------------------------------------------------------
