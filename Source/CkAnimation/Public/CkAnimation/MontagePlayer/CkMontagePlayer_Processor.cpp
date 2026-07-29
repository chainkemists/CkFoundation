#include "CkMontagePlayer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkAnimation/CkAnimation_Utils.h"
#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <Animation/AnimInstance.h>
#include <Animation/AnimMontage.h>
#include <Components/SkeletalMeshComponent.h>
#include <GameFramework/PlayerState.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_MonitorAnimInstance);
CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_Replicate);

const FCk_Time ck::FProcessor_MontagePlayer_HandleRequests::_SyncTargetTime{0.5};

namespace ck_montageplayer_processor
{
    auto Get_IsPreloadPending(
        const ck::FFragment_MontagePlayer_Requests::RequestType& InEntry) -> bool
    {
        const auto* Play = std::get_if<FCk_Request_MontagePlayer_Play>(&InEntry);
        if (Play == nullptr)
        { return false; }
        const auto& Batch = Play->Get_PreloadBatch();
        return Batch.Get_IsRequested() && NOT Batch.Get_IsReady();
    }

    // Batch-first so the played montage is the one the batch roots; the resident-or-null fallback
    // covers requests that never kicked a batch (built raw in BP/AS, or replication rebuilds).
    auto Resolve_Montage(
        const FCk_Request_MontagePlayer_Play& InRequest) -> UAnimMontage*
    {
        const auto& Batch = InRequest.Get_PreloadBatch();
        if (Batch.Get_IsRequested())
        { return Cast<UAnimMontage>(Batch.Get_ResolvedObject(InRequest.Get_Montage().ToSoftObjectPath())); }
        return InRequest.Get_Montage().Get();
    }

    // Only a Play carries an OnFinished contract; the other request kinds have nothing to report.
    auto Broadcast_PlayFailed(
        FCk_Handle_MontagePlayer InHandle,
        const FCk_Request_MontagePlayer_Play& InRequest,
        ECk_MontagePlayer_FinishReason InReason) -> void
    {
        ck::UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
            InHandle, ck::MakePayload(InHandle, FCk_MontagePlayer_State{Resolve_Montage(InRequest)}, InReason));
    }

    template <typename T_Request>
    auto Broadcast_PlayFailed(
        FCk_Handle_MontagePlayer,
        const T_Request&,
        ECk_MontagePlayer_FinishReason) -> void
    {}
}

namespace ck
{
    static auto
        Get_NowFor(
            const FCk_Handle& InHandle) -> FCk_Time
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return FCk_Time::ZeroSecond(); }

        return FCk_WorldTime{World}.Get_Time();
    }

    static auto
        Get_HalfRtt(
            UWorld* InWorld) -> FCk_Time
    {
        if (ck::Is_NOT_Valid(InWorld))
        { return FCk_Time::ZeroSecond(); }

        const auto* PC = InWorld->GetFirstPlayerController();
        if (ck::Is_NOT_Valid(PC))
        { return FCk_Time::ZeroSecond(); }

        const auto* PS = PC->GetPlayerState<APlayerState>();
        if (ck::Is_NOT_Valid(PS))
        { return FCk_Time::ZeroSecond(); }

        return FCk_Time{static_cast<double>(PS->ExactPing) / 1000.0 / 2.0};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_MontagePlayer_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Params& InParams,
            FFragment_MontagePlayer_Current& InCurrent,
            FFragment_MontagePlayer_Requests& InRequestsComp) const
        -> void
    {
        auto* SkelMeshComp = InParams.Get_Params().Get_SkeletalMeshComponent().Get();
        const auto SkelMeshCompIsValid = ck::IsValid(SkelMeshComp);
        CK_ENSURE_IF_NOT(SkelMeshCompIsValid,
            TEXT("SkeletalMeshComponent on MontagePlayer [{}] is no longer valid."), InHandle)
        {
            DoFailPendingRequests(InHandle, InRequestsComp, ECk_PlayMontageFailureReason::InvalidMeshComponent);
            return;
        }

        auto* AI = SkelMeshComp->GetAnimInstance();
        const auto AnimInstanceIsValid = ck::IsValid(AI);
        CK_ENSURE_IF_NOT(AnimInstanceIsValid,
            TEXT("AnimInstance on SkelMesh for MontagePlayer [{}] is invalid."), InHandle)
        {
            DoFailPendingRequests(InHandle, InRequestsComp,
                ECk_PlayMontageFailureReason::MissingAnimInstanceOnMeshComponent);
            return;
        }

        InCurrent._LastSeenAnimInstance = AI;

        // While the head Play's preload batch is still loading, the fragment is left untouched so
        // everything behind the head stays queued in order; the tag is not this drain's dirty marker
        // (the Requests fragment is), so re-marking it cannot re-pump the drain within the frame.
        if (InRequestsComp._Requests.Num() > 0 &&
            ck_montageplayer_processor::Get_IsPreloadPending(InRequestsComp._Requests[0]))
        {
            InHandle.AddOrGet<FTag_MontagePlayer_PendingAssetLoad>();
            return;
        }

        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_MontagePlayer_Requests& InRequests)
        {
            for (auto Index = 0; Index < InRequests._Requests.Num(); ++Index)
            {
                if (ck_montageplayer_processor::Get_IsPreloadPending(InRequests._Requests[Index]))
                {
                    // Splicing the remainder to the FRONT keeps it ahead of re-entrant requests that
                    // handler signals enqueued later; spliced entries keep their completion delegates
                    // bound for a later drain, or for teardown's Failed_Cancelled.
                    InHandle.AddOrGet<FFragment_MontagePlayer_Requests>()._Requests.Insert(
                        InRequests._Requests.GetData() + Index, InRequests._Requests.Num() - Index, 0);
                    InHandle.AddOrGet<FTag_MontagePlayer_PendingAssetLoad>();
                    return;
                }

                std::visit([&](const auto& InRequest) -> void
                {
                    auto Result = ECk_Request_OperationResult::Failed;
                    const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                    Result = DoHandleRequest(InHandle, AI, InCurrent, InRequest);

                    if (InRequest.Get_IsRequestHandleValid())
                    {
                        InRequest.GetAndDestroyRequestHandle();
                    }
                }, InRequests._Requests[Index]);
            }
        });

        InHandle.Try_Remove<FTag_MontagePlayer_PendingAssetLoad>();
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoFailPendingRequests(
            HandleType InHandle,
            FFragment_MontagePlayer_Requests& InRequestsComp,
            ECk_PlayMontageFailureReason InFailureReason)
        -> void
    {
        // Never strand a caller: a drain abort completes every queued request as Failed instead of
        // silently dropping bound completion delegates, and mirrors the enqueue-time pre-flight's
        // OnFinished broadcast so a consumer bound only to that signal still observes the failure.
        const auto FinishReason = montage_player_detail::MapFailureReason(InFailureReason);

        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_MontagePlayer_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                ck_montageplayer_processor::Broadcast_PlayFailed(InHandle, InRequest, FinishReason);
                InRequest.TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed);
            }));
        });
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Play& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& PreloadBatch = InRequest.Get_PreloadBatch();
        const auto PreloadSucceeded = NOT (PreloadBatch.Get_IsRequested() && PreloadBatch.Get_HasFailed());
        CK_ENSURE_IF_NOT(PreloadSucceeded,
            TEXT("MontagePlayer [{}]: preload of Montage [{}] failed — completing the request as Failed"),
            InHandle, InRequest.Get_Montage().ToSoftObjectPath().ToString())
        { return ECk_Request_OperationResult::Failed; }

        auto* Montage = ck_montageplayer_processor::Resolve_Montage(InRequest);
        if (ck::Is_NOT_Valid(Montage))
        { return ECk_Request_OperationResult::Failed; }

        if (InRequest.Get_PreflightDeferred())
        {
            // Enqueue could not validate a not-yet-resident montage; re-run the full pre-flight now,
            // mirroring the synchronous failure shape (OnFinished broadcast with the mapped reason).
            auto Validation = ECk_SucceededFailed::Failed;
            const auto Failure = UCk_Utils_Animation_UE::Get_CanPlayMontage(
                InAnimInstance->GetOwningComponent(), Montage, InRequest.Get_PlayRate(), Validation);

            if (Validation == ECk_SucceededFailed::Failed)
            {
                const auto Reason = montage_player_detail::MapFailureReason(Failure);
                UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                    InHandle, ck::MakePayload(InHandle, FCk_MontagePlayer_State{Montage}, Reason));
                return ECk_Request_OperationResult::Failed;
            }
        }

        const auto FromReplication = InRequest.Get_FromReplication();

        const auto PreviousMontage = InCurrent._ActiveMontage.Get();
        if (ck::IsValid(PreviousMontage) && PreviousMontage != Montage)
        {
            const auto PreviousState = InCurrent._State;
            InAnimInstance->Montage_Stop(static_cast<float>(PreviousState.Get_BlendOutTime().Get_Seconds()), PreviousMontage);
            UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                InHandle, ck::MakePayload(InHandle, PreviousState, ECk_MontagePlayer_FinishReason::Interrupted));
        }

        auto NewState = FCk_MontagePlayer_State{Montage};
        NewState
            .Set_SectionName(InRequest.Get_SectionName())
            .Set_StartPosition(InRequest.Get_StartPosition())
            .Set_PlayRate(InRequest.Get_PlayRate())
            .Set_BlendInTime(InRequest.Get_BlendInTime())
            .Set_BlendOutTime(InRequest.Get_BlendOutTime())
            .Set_Kind(ECk_MontagePlayer_StateKind::Play);

        if (FromReplication)
        {
            NewState
                .Set_PlayInstanceId(InRequest.Get_AuthoritativePlayInstanceId())
                .Set_ServerStartTime(InRequest.Get_AuthoritativeServerStartTime());
        }
        else
        {
            NewState
                .Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1)
                .Set_ServerStartTime(Get_NowFor(InHandle));
        }

        auto StartPos = NewState.Get_StartPosition();
        auto CatchUpRate = 1.0f;
        auto BurnCatchUp = false;

        if (FromReplication)
        {
            auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            const auto Now = Get_NowFor(InHandle);
            const auto HalfRtt = Get_HalfRtt(World);
            const auto Elapsed = (Now - NewState.Get_ServerStartTime()) - HalfRtt;
            const auto MontageLength = FCk_Time{static_cast<double>(Montage->GetPlayLength())};

            if (Elapsed >= MontageLength)
            {
                // Silent drop: server already finished — neither Started nor Finished fires. The
                // montage ends up stopped, which is what the replicated state asked for, so this
                // counts as honoured rather than failed.
                InCurrent._State = NewState;
                InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Stop);
                InCurrent._ActiveMontage = nullptr;
                InCurrent._CatchUpRemaining = FCk_Time::ZeroSecond();
                InHandle.Remove<FTag_MontagePlayer_HasActiveMontage>();
                return ECk_Request_OperationResult::Succeeded;
            }

            if (Elapsed > FCk_Time::ZeroSecond())
            {
                StartPos = NewState.Get_StartPosition() + Elapsed;
                if (Elapsed < _SyncTargetTime)
                {
                    const auto Ratio = static_cast<float>(Elapsed.Get_Seconds() / _SyncTargetTime.Get_Seconds());
                    CatchUpRate = FMath::Min(_MaxPlayRateAdjustment, 1.0f + Ratio);
                    BurnCatchUp = true;
                }
            }
        }

        const auto PlayRate = NewState.Get_PlayRate() * CatchUpRate;
        const auto Length = InAnimInstance->Montage_Play(
            Montage, PlayRate, EMontagePlayReturnType::MontageLength, static_cast<float>(StartPos.Get_Seconds()));

        if (Length <= 0.0f)
        {
            UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                InHandle, ck::MakePayload(InHandle, NewState, ECk_MontagePlayer_FinishReason::Failed_SlotMismatch));
            InCurrent._ActiveMontage = nullptr;
            InCurrent._CatchUpRemaining = FCk_Time::ZeroSecond();
            InHandle.Remove<FTag_MontagePlayer_HasActiveMontage>();
            return ECk_Request_OperationResult::Failed;
        }

        if (NewState.Get_SectionName() != NAME_None)
        {
            InAnimInstance->Montage_JumpToSection(NewState.Get_SectionName(), Montage);
        }

        InCurrent._CatchUpRemaining = BurnCatchUp ? _SyncTargetTime : FCk_Time::ZeroSecond();

        const auto Snapshot = NewState;
        auto HandleCopy = InHandle;
        auto Lambda = FOnMontageEnded::CreateLambda(
            [HandleCopy, Snapshot](UAnimMontage* /*InMontage*/, bool InInterrupted) mutable
            {
                const auto Reason = InInterrupted
                    ? ECk_MontagePlayer_FinishReason::Interrupted
                    : ECk_MontagePlayer_FinishReason::Completed;
                UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                    HandleCopy, ck::MakePayload(HandleCopy, Snapshot, Reason));
            });
        InAnimInstance->Montage_SetEndDelegate(Lambda, Montage);

        InCurrent._State = NewState;
        InCurrent._ActiveMontage = Montage;
        InHandle.AddOrGet<FTag_MontagePlayer_HasActiveMontage>();

        UUtils_Signal_MontagePlayer_OnStarted::Broadcast(InHandle, ck::MakePayload(InHandle, Snapshot));

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Stop& InRequest)
        -> ECk_Request_OperationResult
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        {
            // Idempotent: nothing is playing, so the desired end state (stopped) already holds.
            return ECk_Request_OperationResult::Succeeded;
        }

        const auto FromReplication = InRequest.Get_FromReplication();

        InAnimInstance->Montage_Stop(static_cast<float>(InRequest.Get_BlendOutTime().Get_Seconds()), Montage);

        InCurrent._State.Set_BlendOutTime(InRequest.Get_BlendOutTime());
        InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Stop);

        if (FromReplication)
        { InCurrent._State.Set_PlayInstanceId(InRequest.Get_AuthoritativePlayInstanceId()); }
        else
        { InCurrent._State.Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1); }

        InCurrent._ActiveMontage = nullptr;
        InCurrent._CatchUpRemaining = FCk_Time::ZeroSecond();
        InHandle.Remove<FTag_MontagePlayer_HasActiveMontage>();

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Pause& InRequest)
        -> ECk_Request_OperationResult
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        {
            // Nothing is playing — there is no montage to pause, so the caller's intent cannot be
            // honoured (missing target), unlike Stop's already-stopped no-op.
            return ECk_Request_OperationResult::Failed;
        }

        const auto FromReplication = InRequest.Get_FromReplication();

        if (auto* MontageInstance = InAnimInstance->GetActiveInstanceForMontage(Montage))
        { MontageInstance->Pause(); }

        InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Pause);

        if (FromReplication)
        { InCurrent._State.Set_PlayInstanceId(InRequest.Get_AuthoritativePlayInstanceId()); }
        else
        { InCurrent._State.Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1); }

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Resume& InRequest)
        -> ECk_Request_OperationResult
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        {
            // Nothing is playing — there is no montage to resume (missing target).
            return ECk_Request_OperationResult::Failed;
        }

        const auto FromReplication = InRequest.Get_FromReplication();

        InAnimInstance->Montage_Resume(Montage);

        InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Resume);

        if (FromReplication)
        { InCurrent._State.Set_PlayInstanceId(InRequest.Get_AuthoritativePlayInstanceId()); }
        else
        { InCurrent._State.Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1); }

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_JumpToSection& InRequest)
        -> ECk_Request_OperationResult
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        {
            // Nothing is playing — there is no section to jump within (missing target).
            return ECk_Request_OperationResult::Failed;
        }

        const auto FromReplication = InRequest.Get_FromReplication();

        InAnimInstance->Montage_JumpToSection(InRequest.Get_SectionName(), Montage);

        InCurrent._State.Set_SectionName(InRequest.Get_SectionName());
        InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::JumpToSection);

        if (FromReplication)
        { InCurrent._State.Set_PlayInstanceId(InRequest.Get_AuthoritativePlayInstanceId()); }
        else
        { InCurrent._State.Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1); }

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }

        return ECk_Request_OperationResult::Succeeded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_MontagePlayer_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_MontagePlayer_MonitorAnimInstance::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Params& InParams,
            FFragment_MontagePlayer_Current& InCurrent) const
        -> void
    {
        auto* SkelMeshComp = InParams.Get_Params().Get_SkeletalMeshComponent().Get();
        if (ck::Is_NOT_Valid(SkelMeshComp))
        { return; }

        auto* CurrentAI = SkelMeshComp->GetAnimInstance();
        const auto* LastSeenAI = InCurrent._LastSeenAnimInstance.Get();

        if (CurrentAI != LastSeenAI && ck::IsValid(LastSeenAI))
        {
            const auto Snapshot = InCurrent._State;
            UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                InHandle, ck::MakePayload(InHandle, Snapshot, ECk_MontagePlayer_FinishReason::Interrupted));

            InCurrent._ActiveMontage = nullptr;
            InCurrent._LastSeenAnimInstance = CurrentAI;
            InCurrent._CatchUpRemaining = FCk_Time::ZeroSecond();
            InHandle.Remove<FTag_MontagePlayer_HasActiveMontage>();

            if (UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
            {
                InCurrent._State.Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1);
                InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Stop);
                UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle);
            }

            return;
        }

        if (InCurrent._CatchUpRemaining > FCk_Time::ZeroSecond())
        {
            InCurrent._CatchUpRemaining -= InDeltaT;
            if (InCurrent._CatchUpRemaining <= FCk_Time::ZeroSecond())
            {
                InCurrent._CatchUpRemaining = FCk_Time::ZeroSecond();
                if (auto* Montage = InCurrent._ActiveMontage.Get(); ck::IsValid(Montage) && ck::IsValid(CurrentAI))
                {
                    CurrentAI->Montage_SetPlayRate(Montage, 1.0f);
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_MontagePlayer_Replicate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_MontagePlayer_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Current& InCurrent,
            const FFragment_ContainerRef_MontagePlayer& InRepRef) const
        -> void
    {
        auto* Driver = InRepRef.Get_Driver().Get();
        if (ck::Is_NOT_Valid(Driver))
        { return; }

        const auto Produced = UCk_Utils_Net_UE::TryProduce<FCk_RepData_MontagePlayer>(InHandle);
        if (Produced.IsSet())
        { Driver->SetFragmentData<FCk_RepData_MontagePlayer>(*Produced); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
