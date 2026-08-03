#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoiceChat/VoiceTalker/CkVoiceTalker_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVOICECHAT_API FProcessor_VoiceTalker_Setup : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_Setup,
        FCk_Handle_VoiceTalker,
        ck::TReadOnly<FFragment_VoiceTalker_Params>,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        FTag_VoiceTalker_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_VoiceTalker_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVOICECHAT_API FProcessor_VoiceTalker_HandleRequests : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_HandleRequests,
        FCk_Handle_VoiceTalker,
        ck::TReadOnly<FFragment_VoiceTalker_Params>,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        ck::TReadWrite<FFragment_VoiceTalker_Requests>,
        TExclude<FTag_VoiceTalker_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceTalker_Setup>;
        using MarkedDirtyBy = FFragment_VoiceTalker_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            FFragment_VoiceTalker_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_StartTransmit& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_StopTransmit& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_SetTransmitMode& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_SetInputGain& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent,
            const FCk_Request_VoiceTalker_SetSelfMute& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed talker's
    // still-queued requests are never drained; this fires each pending request's completion
    // delegate with Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKVOICECHAT_API FProcessor_VoiceTalker_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_CancelPendingRequests,
        FCk_Handle_VoiceTalker,
        ck::TReadOnly<FFragment_VoiceTalker_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Requests& InRequests)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drives the capture pipeline while transmitting: drain -> gain -> per-frame RMS/VAD -> Opus
    // encode -> loopback playout. Time-stepping, so deliberately no MarkedDirtyBy (not
    // pump-eligible).
    class CKVOICECHAT_API FProcessor_VoiceTalker_Capture : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_Capture,
        FCk_Handle_VoiceTalker,
        ck::TReadOnly<FFragment_VoiceTalker_Params>,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        FTag_VoiceTalker_IsTransmitting,
        TExclude<FTag_VoiceTalker_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceTalker_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            const FFragment_VoiceTalker_Params& InParams,
            FFragment_VoiceTalker_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Plays out a REMOTE talker's forwarded voice on this machine: drains the receive inbox into
    // the talker's jitter buffer, then runs the shared playout drain (pop -> decode -> PCM ->
    // synth). Runs on every net mode - a listen host receives other players' voice like any
    // client. Time-stepping (conceal and tail drain continue without new bundles), so
    // deliberately no MarkedDirtyBy.
    class CKVOICECHAT_API FProcessor_VoiceTalker_ReceivePlayback : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_ReceivePlayback,
        FCk_Handle_VoiceTalker,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        ck::TReadWrite<FFragment_VoiceTalker_ReceiveInbox>,
        TExclude<FTag_VoiceTalker_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceTalker_Capture>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Current& InCurrent,
            FFragment_VoiceTalker_ReceiveInbox& InInbox)
            -> void;

    public:
        // The one playout drain both consumers share (Capture's loopback and this processor's
        // receive path): advances the pop accumulator, pops/conceals/decodes into the decoded-PCM
        // buffer + synth, and trims the buffer. No-op (accumulator cleared) without a decoder.
        static auto
        Drain_Playout(
            FFragment_VoiceTalker_Current& InCurrent,
            FCk_Time InDeltaT)
            -> void;

        // Creates + starts the playback synth if the world can render audio; no-op otherwise
        // (headless PIE) - the decode path still fills the test-readable PCM buffer.
        static auto
        TryCreate_PlaybackSynth(
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Teardown order per spec §7.8: stop capture BEFORE releasing the encoder/decoder.
    class CKVOICECHAT_API FProcessor_VoiceTalker_EndPlay : public ck_exp::TProcessor<
        FProcessor_VoiceTalker_EndPlay,
        FCk_Handle_VoiceTalker,
        ck::TReadWrite<FFragment_VoiceTalker_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceTalkerEntity,
            FFragment_VoiceTalker_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
