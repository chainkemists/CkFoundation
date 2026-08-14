#include "CkVoiceListener_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/Net/CkVoiceChatControlRelay_Actor.h"
#include "CkVoiceChat/Net/CkVoiceChatControlRelay_Subsystem.h"

#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceListener_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceListener_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceListener_SyncMutes);
CK_REGISTER_PROCESSOR(ck::FProcessor_VoiceChat_ApplyControl);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_listener_processor
{
    auto
    Get_LocalPlayerState(
        UWorld* InWorld) -> APlayerState*
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return nullptr; }

        const auto* LocalController = InWorld->GetFirstPlayerController();

        if (ck::Is_NOT_Valid(LocalController))
        { return nullptr; }

        return LocalController->PlayerState;
    }

    auto
    ResolveControlRelay_ForPlayer(
        UWorld* InWorld,
        APlayerState* InPlayerState) -> ACk_VoiceChatControlRelay_UE*
    {
        if (ck::Is_NOT_Valid(InWorld) || ck::Is_NOT_Valid(InPlayerState))
        { return nullptr; }

        auto* Subsystem = InWorld->GetSubsystem<UCk_VoiceChatControlRelay_Subsystem_UE>();

        if (ck::Is_NOT_Valid(Subsystem))
        { return nullptr; }

        auto Pending = Subsystem->Request_AcquireChannel_ForPlayer(InPlayerState);
        const auto Result = Subsystem->Try_ResolvePending(Pending);

        return ::Cast<ACk_VoiceChatControlRelay_UE>(Result.Get_ChannelActor().Get());
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VoiceListener_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceListenerEntity,
            FFragment_VoiceListener_Current& InCurrent,
            FFragment_VoiceListener_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InVoiceListenerEntity, Result);

            if (DoHandleRequest(InVoiceListenerEntity, InCurrent, InRequest))
            {
                Result = ECk_Request_OperationResult::Succeeded;
            }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InVoiceListenerEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_VoiceListener_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceListener_Current& InCurrent,
            const FCk_Request_VoiceListener_MuteTalker& InRequest)
        -> bool
    {
        const auto& Talker = InRequest.Get_Talker();

        const auto TalkerIsValid = ck::IsValid(Talker);
        CK_ENSURE_IF_NOT(TalkerIsValid,
            TEXT("MuteTalker on VoiceListener [{}] with an invalid Talker handle"), InHandle)
        { return false; }

        auto AlreadyMuted = false;
        InCurrent._MutedTalkers.Add(Talker, &AlreadyMuted);

        if (NOT AlreadyMuted)
        {
            InHandle.AddOrGet<FTag_VoiceListener_MutesDirty>();
        }

        return true;
    }

    auto
        FProcessor_VoiceListener_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceListener_Current& InCurrent,
            const FCk_Request_VoiceListener_UnmuteTalker& InRequest)
        -> bool
    {
        if (InCurrent._MutedTalkers.Remove(InRequest.Get_Talker()) > 0)
        {
            InHandle.AddOrGet<FTag_VoiceListener_MutesDirty>();
        }

        return true;
    }

    auto
        FProcessor_VoiceListener_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VoiceListener_Current& InCurrent,
            const FCk_Request_VoiceListener_SetTalkerVolume& InRequest)
        -> bool
    {
        const auto& Talker = InRequest.Get_Talker();

        const auto TalkerIsValid = ck::IsValid(Talker);
        CK_ENSURE_IF_NOT(TalkerIsValid,
            TEXT("SetTalkerVolume on VoiceListener [{}] with an invalid Talker handle"), InHandle)
        { return false; }

        InCurrent._TalkerVolumes.Add(Talker, InRequest.Get_Volume());
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceListener_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceListenerEntity,
            const FFragment_VoiceListener_Requests& InRequests)
        -> void
    {
        ck::request::FireCancelledForPending(InVoiceListenerEntity, InRequests.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceListener_SyncMutes::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVoiceListenerEntity,
            const FFragment_VoiceListener_Current& InCurrent)
        -> void
    {
        using namespace ck_voice_listener_processor;

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVoiceListenerEntity);
        auto* LocalPlayer = Get_LocalPlayerState(World);

        if (ck::Is_NOT_Valid(LocalPlayer))
        { return; }   // no local player yet - keep the dirty tag, retry next tick

        auto MutedTalkers = InCurrent.Get_MutedTalkers().Array();

        if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InVoiceListenerEntity))
        {
            auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InVoiceListenerEntity);
            auto& Matrix = TransientEntity.AddOrGet<FFragment_VoiceChat_ListenerMuteMatrix>();
            Matrix.Get_MutedByPlayer().Add(MakeWeakObjectPtr(LocalPlayer), TSet<FCk_Handle>{MutedTalkers});

            InVoiceListenerEntity.Remove<MarkedDirtyBy>();
            return;
        }

        auto* Relay = ResolveControlRelay_ForPlayer(World, LocalPlayer);

        if (ck::Is_NOT_Valid(Relay))
        { return; }   // relay not resolvable yet - keep the dirty tag, retry next tick

        Relay->Server_SetMutedTalkers(MutedTalkers);
        InVoiceListenerEntity.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VoiceChat_ApplyControl::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTransientEntity,
            FFragment_VoiceChat_ControlInbox& InInbox)
        -> void
    {
        const auto UpdatesCopy = InInbox.Get_Updates();
        InInbox.Get_Updates().Reset();

        if (UpdatesCopy.IsEmpty())
        { return; }

        auto& Matrix = InTransientEntity.AddOrGet<FFragment_VoiceChat_ListenerMuteMatrix>();

        for (const auto& Update : UpdatesCopy)
        {
            if (NOT Update.Get_Player().IsValid())
            { continue; }

            Matrix.Get_MutedByPlayer().Add(Update.Get_Player(), TSet<FCk_Handle>{Update.Get_MutedTalkers()});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
