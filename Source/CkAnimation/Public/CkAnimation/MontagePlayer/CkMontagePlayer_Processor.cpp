#include "CkMontagePlayer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkAnimation/MontagePlayer/CkMontagePlayer_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <Animation/AnimInstance.h>
#include <Animation/AnimMontage.h>
#include <Components/SkeletalMeshComponent.h>
#include <GameFramework/PlayerState.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_MonitorAnimInstance);
CK_REGISTER_PROCESSOR(ck::FProcessor_MontagePlayer_Replicate);

const FCk_Time ck::FProcessor_MontagePlayer_HandleRequests::_SyncTargetTime{0.5};

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
        CK_ENSURE_IF_NOT(ck::IsValid(SkelMeshComp),
            TEXT("SkeletalMeshComponent on MontagePlayer [{}] is no longer valid."), InHandle)
        {
            InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_MontagePlayer_Requests& InRequests) { /* drain */ });
            return;
        }

        auto* AI = SkelMeshComp->GetAnimInstance();
        CK_ENSURE_IF_NOT(ck::IsValid(AI),
            TEXT("AnimInstance on SkelMesh for MontagePlayer [{}] is invalid."), InHandle)
        {
            InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_MontagePlayer_Requests& InRequests) { /* drain */ });
            return;
        }

        InCurrent._LastSeenAnimInstance = AI;

        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_MontagePlayer_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, AI, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
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
        -> void
    {
        auto* Montage = InRequest.Get_Montage().Get();
        if (ck::Is_NOT_Valid(Montage))
        { return; }

        const auto FromReplication = InRequest.Get_FromReplication();

        // Stop any previously-active montage and broadcast its terminal interrupt before we replace state.
        const auto PreviousMontage = InCurrent._ActiveMontage.Get();
        if (ck::IsValid(PreviousMontage) && PreviousMontage != Montage)
        {
            const auto PreviousState = InCurrent._State;
            InAnimInstance->Montage_Stop(static_cast<float>(PreviousState.Get_BlendOutTime().Get_Seconds()), PreviousMontage);
            UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                InHandle, ck::MakePayload(InHandle, PreviousState, ECk_MontagePlayer_FinishReason::Interrupted));
        }

        // Build the new state row.
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

        // Late-join catch-up on replicated plays: advance start position, optionally adjust playrate.
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
                // Silent drop: server already finished — neither Started nor Finished fires.
                InCurrent._State = NewState;
                InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Stop);
                InCurrent._ActiveMontage = nullptr;
                InCurrent._CatchUpRemaining = FCk_Time::ZeroSecond();
                InHandle.Remove<FTag_MontagePlayer_HasActiveMontage>();
                return;
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

        // Drive the AnimInstance.
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
            return;
        }

        if (NewState.Get_SectionName() != NAME_None)
        {
            InAnimInstance->Montage_JumpToSection(NewState.Get_SectionName(), Montage);
        }

        // cancel-on-replace: overwrite remaining catch-up seconds.
        InCurrent._CatchUpRemaining = BurnCatchUp ? _SyncTargetTime : FCk_Time::ZeroSecond();

        // Bind end-delegate per-instance.
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

        // Commit state and announce.
        InCurrent._State = NewState;
        InCurrent._ActiveMontage = Montage;
        InHandle.AddOrGet<FTag_MontagePlayer_HasActiveMontage>();

        UUtils_Signal_MontagePlayer_OnStarted::Broadcast(InHandle, ck::MakePayload(InHandle, Snapshot));

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Stop& InRequest)
        -> void
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        { return; }

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
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Pause& InRequest)
        -> void
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        { return; }

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
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Resume& InRequest)
        -> void
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        { return; }

        const auto FromReplication = InRequest.Get_FromReplication();

        InAnimInstance->Montage_Resume(Montage);

        InCurrent._State.Set_Kind(ECk_MontagePlayer_StateKind::Resume);

        if (FromReplication)
        { InCurrent._State.Set_PlayInstanceId(InRequest.Get_AuthoritativePlayInstanceId()); }
        else
        { InCurrent._State.Set_PlayInstanceId(InCurrent._State.Get_PlayInstanceId() + 1); }

        if (NOT FromReplication)
        { UCk_Utils_MontagePlayer_UE::Request_TryReplicateMontagePlayer(InHandle); }
    }

    auto
        FProcessor_MontagePlayer_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_JumpToSection& InRequest)
        -> void
    {
        auto* Montage = InCurrent._ActiveMontage.Get();
        if (ck::Is_NOT_Valid(Montage))
        { return; }

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

        Driver->SetFragmentData<FCk_RepData_MontagePlayer>(FCk_RepData_MontagePlayer{InCurrent.Get_State()});
    }
}

// --------------------------------------------------------------------------------------------------------------------
