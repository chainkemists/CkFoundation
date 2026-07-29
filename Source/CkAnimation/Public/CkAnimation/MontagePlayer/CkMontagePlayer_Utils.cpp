#include "CkMontagePlayer_Utils.h"

#include "CkAnimation/CkAnimation_Log.h"
#include "CkAnimation/CkAnimation_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkResourceLoader/CkResourceLoader_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MontagePlayer_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_MontagePlayer_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_MontagePlayer
{
    CK_ENSURE_IF_NOT(InParams.Get_SkeletalMeshComponent().IsValid(),
        TEXT("Adding MontagePlayer to Entity [{}] with an INVALID SkeletalMeshComponent!"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("MontagePlayer already exists on Entity [{}] — only one MontagePlayer per entity is supported."), InHandle)
    { return Cast(InHandle); }

    InHandle.Add<ck::FFragment_MontagePlayer_Params>(InParams);
    InHandle.Add<ck::FFragment_MontagePlayer_Current>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::animation::VeryVerbose
        (
            TEXT("Skipping creation of MontagePlayer Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle, InReplicates
        );
    }
    else
    {
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_MontagePlayer>(InHandle);
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_MontagePlayer_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_MontagePlayer_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_MontagePlayer
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams, InReplicates);
}

auto
    UCk_Utils_MontagePlayer_UE::
    Request_RebindSkeletalMeshComponent(
        FCk_Handle_MontagePlayer& InMontagePlayer,
        USkeletalMeshComponent* InSkeletalMeshComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    const auto IsSkeletalMeshComponentValid = ck::IsValid(InSkeletalMeshComponent);
    CK_ENSURE_IF_NOT(IsSkeletalMeshComponentValid,
        TEXT("Rebinding MontagePlayer on Entity [{}] with an INVALID SkeletalMeshComponent!"), InMontagePlayer)
    {
        InDelegate.ExecuteIfBound(InMontagePlayer, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InMontagePlayer;
    }

    // The SKMC is the one param that cannot round-trip a save/load — a live component can't be captured or
    // rebuilt from a snapshot payload — so replace the whole Params payload with one wrapping the re-created mesh.
    auto& ParamsFragment = InMontagePlayer.Get<ck::FFragment_MontagePlayer_Params>();
    ParamsFragment._Params = FCk_Fragment_MontagePlayer_ParamsData{InSkeletalMeshComponent};

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InMontagePlayer, ECk_Request_OperationResult::Succeeded);

    return InMontagePlayer;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_MontagePlayer_UE, FCk_Handle_MontagePlayer, ck::FFragment_MontagePlayer_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MontagePlayer_UE::
    Get_CurrentMontage(
        const FCk_Handle_MontagePlayer& InHandle)
    -> UAnimMontage*
{
    return InHandle.Get<ck::FFragment_MontagePlayer_Current>().Get_ActiveMontage().Get();
}

auto
    UCk_Utils_MontagePlayer_UE::
    Get_CurrentSection(
        const FCk_Handle_MontagePlayer& InHandle)
    -> FName
{
    return InHandle.Get<ck::FFragment_MontagePlayer_Current>().Get_State().Get_SectionName();
}

auto
    UCk_Utils_MontagePlayer_UE::
    Get_IsPlaying(
        const FCk_Handle_MontagePlayer& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_MontagePlayer_HasActiveMontage>();
}

auto
    UCk_Utils_MontagePlayer_UE::
    Get_PlayInstanceId(
        const FCk_Handle_MontagePlayer& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_MontagePlayer_Current>().Get_State().Get_PlayInstanceId();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MontagePlayer_UE::
    Request_Play(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Play& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InHandle);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Play called without authority on MontagePlayer [{}] — request dropped."), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    auto* SkelMeshComp = InHandle.Get<ck::FFragment_MontagePlayer_Params>().Get_Params().Get_SkeletalMeshComponent().Get();

    auto Request = InRequest;

    // Unset and resident keep the old pre-flight; authored-but-not-resident is net-new and defers to
    // the drain, which re-runs it once the batch lands.
    const auto MontageIsAuthored = ck::IsValid(Request.Get_Montage());
    auto* ResolvedMontage = Request.Get_Montage().Get();

    if (NOT MontageIsAuthored || ck::IsValid(ResolvedMontage))
    {
        auto Validation = ECk_SucceededFailed::Failed;
        const auto Failure = UCk_Utils_Animation_UE::Get_CanPlayMontage(
            SkelMeshComp, ResolvedMontage, Request.Get_PlayRate(), Validation);

        if (Validation == ECk_SucceededFailed::Failed)
        {
            const auto Reason = ck::montage_player_detail::MapFailureReason(Failure);
            const auto FailureState = FCk_MontagePlayer_State{ResolvedMontage};
            ck::UUtils_Signal_MontagePlayer_OnFinished::Broadcast(
                InHandle, ck::MakePayload(InHandle, FailureState, Reason));
            InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
            return InHandle;
        }
    }
    else
    {
        Request.Set_PreflightDeferred(true);
    }

    if (MontageIsAuthored)
    {
        // Kicked even when resident: the soft ptr roots nothing, and the batch is what keeps the
        // montage alive from enqueue to drain. Resident kicks complete inline (warm path unchanged).
        Request.Set_PreloadBatch(UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
            TEXT("MontagePlayer.Play"), {Request.Get_Montage().ToSoftObjectPath()}));
    }

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    CK_CALLSTACK_RECORD(ck::FFragment_MontagePlayer_Requests, InHandle);
    InHandle.AddOrGet<ck::FFragment_MontagePlayer_Requests>()._Requests.Emplace(Request);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    Request_Stop(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Stop& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InHandle);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Stop called without authority on MontagePlayer [{}] — request dropped."), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    CK_CALLSTACK_RECORD(ck::FFragment_MontagePlayer_Requests, InHandle);
    InHandle.AddOrGet<ck::FFragment_MontagePlayer_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    Request_Pause(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Pause& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InHandle);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Pause called without authority on MontagePlayer [{}] — request dropped."), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    CK_CALLSTACK_RECORD(ck::FFragment_MontagePlayer_Requests, InHandle);
    InHandle.AddOrGet<ck::FFragment_MontagePlayer_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    Request_Resume(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Resume& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InHandle);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_Resume called without authority on MontagePlayer [{}] — request dropped."), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    CK_CALLSTACK_RECORD(ck::FFragment_MontagePlayer_Requests, InHandle);
    InHandle.AddOrGet<ck::FFragment_MontagePlayer_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    Request_JumpToSection(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_JumpToSection& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InHandle);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_JumpToSection called without authority on MontagePlayer [{}] — request dropped."), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    CK_CALLSTACK_RECORD(ck::FFragment_MontagePlayer_Requests, InHandle);
    InHandle.AddOrGet<ck::FFragment_MontagePlayer_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MontagePlayer_UE::
    BindTo_OnStarted(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnStarted& InDelegate,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_MontagePlayer
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_MontagePlayer_OnStarted, InHandle, InDelegate, InBehavior, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    UnbindFrom_OnStarted(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnStarted& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_MontagePlayer_OnStarted, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    BindTo_OnFinished(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnFinished& InDelegate,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_MontagePlayer
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_MontagePlayer_OnFinished, InHandle, InDelegate, InBehavior, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_MontagePlayer_UE::
    UnbindFrom_OnFinished(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnFinished& InDelegate)
    -> FCk_Handle_MontagePlayer
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_MontagePlayer_OnFinished, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MontagePlayer_UE::
    DoDispatchReplicatedState(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_MontagePlayer_State& InState,
        bool InIsOnAdd)
    -> void
{
    auto& Requests = InHandle.AddOrGet<ck::FFragment_MontagePlayer_Requests>();

    const auto MakePlay = [&](FName SectionOverride) -> FCk_Request_MontagePlayer_Play
    {
        auto Req = FCk_Request_MontagePlayer_Play{InState.Get_Montage()};
        Req
            .Set_SectionName(SectionOverride)
            .Set_StartPosition(InState.Get_StartPosition())
            .Set_PlayRate(InState.Get_PlayRate())
            .Set_BlendInTime(InState.Get_BlendInTime())
            .Set_BlendOutTime(InState.Get_BlendOutTime())
            .Set_AuthoritativePlayInstanceId(InState.Get_PlayInstanceId())
            .Set_AuthoritativeServerStartTime(InState.Get_ServerStartTime())
            .Set_FromReplication(true);
        return Req;
    };

    switch (InState.Get_Kind())
    {
        case ECk_MontagePlayer_StateKind::Play:
        {
            Requests._Requests.Emplace(MakePlay(InState.Get_SectionName()));
            break;
        }
        case ECk_MontagePlayer_StateKind::Stop:
        {
            if (InIsOnAdd)
            { return; }

            auto Req = FCk_Request_MontagePlayer_Stop{InState.Get_BlendOutTime()};
            Req
                .Set_AuthoritativePlayInstanceId(InState.Get_PlayInstanceId())
                .Set_FromReplication(true);
            Requests._Requests.Emplace(Req);
            break;
        }
        case ECk_MontagePlayer_StateKind::Pause:
        {
            if (InIsOnAdd)
            { Requests._Requests.Emplace(MakePlay(InState.Get_SectionName())); }

            auto PauseReq = FCk_Request_MontagePlayer_Pause{};
            PauseReq
                .Set_AuthoritativePlayInstanceId(InState.Get_PlayInstanceId())
                .Set_FromReplication(true);
            Requests._Requests.Emplace(PauseReq);
            break;
        }
        case ECk_MontagePlayer_StateKind::Resume:
        {
            if (InIsOnAdd)
            {
                Requests._Requests.Emplace(MakePlay(InState.Get_SectionName()));
                return;
            }

            auto Req = FCk_Request_MontagePlayer_Resume{};
            Req
                .Set_AuthoritativePlayInstanceId(InState.Get_PlayInstanceId())
                .Set_FromReplication(true);
            Requests._Requests.Emplace(Req);
            break;
        }
        case ECk_MontagePlayer_StateKind::JumpToSection:
        {
            if (InIsOnAdd)
            { Requests._Requests.Emplace(MakePlay(NAME_None)); }

            auto Req = FCk_Request_MontagePlayer_JumpToSection{InState.Get_SectionName()};
            Req
                .Set_AuthoritativePlayInstanceId(InState.Get_PlayInstanceId())
                .Set_FromReplication(true);
            Requests._Requests.Emplace(Req);
            break;
        }
    }
}

auto
    UCk_Utils_MontagePlayer_UE::
    Request_TryReplicateMontagePlayer(
        FCk_Handle_MontagePlayer& InHandle)
    -> void
{
    InHandle.AddOrGet<ck::FTag_MontagePlayer_MayRequireReplication>();
}

// --------------------------------------------------------------------------------------------------------------------
