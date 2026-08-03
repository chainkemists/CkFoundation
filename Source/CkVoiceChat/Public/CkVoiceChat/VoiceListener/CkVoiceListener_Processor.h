#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVOICECHAT_API FProcessor_VoiceListener_HandleRequests : public ck_exp::TProcessor<
        FProcessor_VoiceListener_HandleRequests,
        FCk_Handle_VoiceListener,
        ck::TReadWrite<FFragment_VoiceListener_Current>,
        ck::TReadWrite<FFragment_VoiceListener_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FFragment_VoiceListener_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceListenerEntity,
            FFragment_VoiceListener_Current& InCurrent,
            FFragment_VoiceListener_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceListener_Current& InCurrent,
            const FCk_Request_VoiceListener_MuteTalker& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceListener_Current& InCurrent,
            const FCk_Request_VoiceListener_UnmuteTalker& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceListener_Current& InCurrent,
            const FCk_Request_VoiceListener_SetTalkerVolume& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed listener's
    // still-queued requests are never drained; this fires each pending request's completion
    // delegate with Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKVOICECHAT_API FProcessor_VoiceListener_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_VoiceListener_CancelPendingRequests,
        FCk_Handle_VoiceListener,
        ck::TReadOnly<FFragment_VoiceListener_Requests>,
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
            HandleType InVoiceListenerEntity,
            const FFragment_VoiceListener_Requests& InRequests)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Reports the local mute set upstream as a full-set replace: a host applies it straight into
    // the routing mute matrix; a client sends it over the reliable CONTROL relay (retrying while
    // the relay is unresolved - the dirty tag stays until the report lands somewhere).
    class CKVOICECHAT_API FProcessor_VoiceListener_SyncMutes : public ck_exp::TProcessor<
        FProcessor_VoiceListener_SyncMutes,
        FCk_Handle_VoiceListener,
        ck::TReadOnly<FFragment_VoiceListener_Current>,
        FTag_VoiceListener_MutesDirty,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VoiceListener_HandleRequests>;
        using MarkedDirtyBy = FTag_VoiceListener_MutesDirty;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceListenerEntity,
            const FFragment_VoiceListener_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applies queued mute-set reports into the routing mute matrix (both live on the transient
    // entity, so the view iterates exactly one entity per world).
    class CKVOICECHAT_API FProcessor_VoiceChat_ApplyControl : public ck_exp::TProcessor<
        FProcessor_VoiceChat_ApplyControl,
        FCk_Handle,
        ck::TReadWrite<FFragment_VoiceChat_ControlInbox>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FFragment_VoiceChat_ControlInbox;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTransientEntity,
            FFragment_VoiceChat_ControlInbox& InInbox)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
