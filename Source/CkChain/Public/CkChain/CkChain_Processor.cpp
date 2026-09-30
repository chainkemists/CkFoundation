#include "CkChain_Processor.h"
#include "CkChain/CkChain_Utils.h"
#include "CkChain/CkChainLink_Utils.h"
#include "CkChain/CkChain_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Chain_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Chain_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Chain_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Chain_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Chain_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_ChainLink_EndPlay);

namespace ck_chain_processor
{
    auto
        DoSortRoster(
            TArray<FCk_Handle_ChainLink>& InLinks) -> void
    {
        InLinks.StableSort([](const FCk_Handle_ChainLink& InA, const FCk_Handle_ChainLink& InB)
        {
            return UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(InA) < UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(InB);
        });
    }

    auto
        DoReserveHistory(
            ck::chain::FPathHistory& InHistory,
            const TArray<FCk_Handle_ChainLink>& InLinks,
            const FCk_Chain_Spec& InParams) -> void
    {
        if (InParams.Get_Solver() != ECk_Chain_Solver::PathHistory)
        { return; }
        const auto Length = InLinks.IsEmpty() ? 0.0f : UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(InLinks.Last());
        InHistory.Reserve_ForDistance(Length + 2.0f * InParams.Get_SampleSpacingCm(), InParams.Get_SampleSpacingCm());
    }

    auto
        DoIsLiveLink(
            const FCk_Handle_ChainLink& InLink,
            const FCk_Handle_Chain& InChain) -> bool
    {
        return ck::IsValid(InLink) && UCk_Utils_ChainLink_UE::Has(InLink)
            && InLink.Get<ck::FFragment_ChainLink>().Get_Chain() == InChain;
    }

    auto
        DoPruneRoster(
            FCk_Handle_Chain InHandle,
            TArray<FCk_Handle_ChainLink>& InLinks) -> void
    {
        const auto Pruned = InLinks.RemoveAll([&](const FCk_Handle_ChainLink& InLink)
        {
            return NOT DoIsLiveLink(InLink, InHandle);
        });
        if (Pruned > 0)
        { InHandle.AddOrGet<ck::FTag_Chain_RosterDirty>(); }
    }
}

auto
    ck::FProcessor_Chain_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent)
    -> void
{
    const auto IsValidHead = ck::IsValid(InCurrent.Get_Head());
    CK_ENSURE_IF_NOT(IsValidHead, TEXT("Chain [{}] has an invalid head"), InHandle)
    { return; }
    InCurrent._UpVectorNormalized = InParams.Get_UpVector().GetSafeNormal();
    const auto HeadPose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InCurrent.Get_Head());
    if (InParams.Get_Solver() == ECk_Chain_Solver::PathHistory)
    {
        InCurrent._History.Reseed(HeadPose, InParams.Get_SampleSpacingCm());
        ck_chain_processor::DoReserveHistory(InCurrent._History, InCurrent._Links, InParams);
    }
    InCurrent._LastHeadTransform = HeadPose;
    if (InParams.Get_StartingState() == ECk_EnableDisable::Disable)
    { InHandle.AddOrGet<FTag_Chain_Disabled>(); }
    InHandle.Remove<MarkedDirtyBy>();
}

auto
    ck::FProcessor_Chain_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        FFragment_Chain_Requests& InRequests)
    -> void
{
    ck_chain_processor::DoPruneRoster(InHandle, InCurrent._Links);
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    algo::ForEachRequest(RequestsCopy, ck::Visitor([&](const auto& InRequest)
    {
        auto Result = ECk_Request_OperationResult::Failed;
        const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);
        if (InHandle.Has<FTag_DestroyEntity_Initiate>())
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(InRequest)>, FCk_Request_Chain_Split>)
            {
                auto NewChain = InRequest.Get_NewChain();
                if (ck::IsValid(NewChain))
                { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewChain); }
            }
            Result = ECk_Request_OperationResult::Failed_Cancelled;
            return;
        }
        ck_chain_processor::DoPruneRoster(InHandle, InCurrent._Links);
        chain::VeryVerbose(TEXT("Handling Chain request on [{}]"), InHandle);
        Result = DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
    }), policy::DontResetContainer{});
    if (InRequests._Requests.IsEmpty())
    { InHandle.Remove<MarkedDirtyBy>(); }
}

auto
    ck::FProcessor_Chain_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        const FCk_Request_Chain_AttachLink& InRequest)
    -> ECk_Request_OperationResult
{
    auto Link = InRequest.Get_Link();
    if (ck::Is_NOT_Valid(Link) || NOT UCk_Utils_Transform_UE::Has(Link))
    {
        chain::Log(TEXT("Chain [{}] attach failed because link is no longer valid"), InHandle);
        return ECk_Request_OperationResult::Failed;
    }
    const auto CanAttach = Link != InCurrent.Get_Head() && NOT UCk_Utils_ChainLink_UE::Has(Link)
        && NOT UCk_Utils_SceneNode_UE::Has(Link)
        && (NOT Link.Has<FFragment_Transform_RootComponent>() || Link.Has<FTag_Transform_Movable>())
        && (InParams.Get_NetPolicy() != ECk_Chain_NetPolicy::Everywhere || NOT Link.Has<FFragment_ContainerRef_Location>());
    if (NOT CanAttach)
    {
        chain::Log(TEXT("Chain [{}] attach failed because link [{}] became incompatible before drain"), InHandle, Link);
        return ECk_Request_OperationResult::Failed;
    }
    const auto& Spec = InRequest.Get_LinkSpec();
    Link.Add<FFragment_ChainLink_Params>(Spec.Get_LocalOffset(), Spec.Get_Orientation());
    Link.Add<FFragment_ChainLink>(InHandle, Spec.Get_DistanceFromHeadCm());
    auto ChainLink = UCk_Utils_ChainLink_UE::CastChecked(Link);
    InCurrent._Links.Add(ChainLink);
    ck_chain_processor::DoSortRoster(InCurrent._Links);
    ck_chain_processor::DoReserveHistory(InCurrent._History, InCurrent._Links, InParams);
    InHandle.AddOrGet<FTag_Chain_RosterDirty>();
    const auto Index = InCurrent._Links.IndexOfByKey(ChainLink);
    UUtils_Signal_OnChainLinkAttached::Broadcast(InHandle, MakePayload(InHandle, FCk_Chain_Payload_LinkAttached{ChainLink, Index}));
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Chain_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        const FCk_Request_Chain_DetachLink& InRequest)
    -> ECk_Request_OperationResult
{
    auto Link = InRequest.Get_Link();
    const auto Index = InCurrent._Links.IndexOfByKey(Link);
    if (Index == INDEX_NONE || NOT ck_chain_processor::DoIsLiveLink(Link, InHandle))
    {
        chain::Log(TEXT("Chain [{}] detach failed for non-member link [{}]"), InHandle, Link);
        return ECk_Request_OperationResult::Failed;
    }
    InCurrent._Links.RemoveAt(Index);
    Link.Try_Remove<FFragment_ChainLink_Params>();
    Link.Try_Remove<FFragment_ChainLink>();
    Link.Try_Remove<FFragment_ChainLink_TargetPose>();
    InHandle.AddOrGet<FTag_Chain_RosterDirty>();
    UUtils_Signal_OnChainLinkDetached::Broadcast(InHandle, MakePayload(InHandle,
        FCk_Chain_Payload_LinkDetached{Link, ECk_Chain_LinkDetachReason::Requested}));
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Chain_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        const FCk_Request_Chain_SetLinkDistance& InRequest)
    -> ECk_Request_OperationResult
{
    auto Link = InRequest.Get_Link();
    if (NOT InCurrent._Links.Contains(Link) || NOT ck_chain_processor::DoIsLiveLink(Link, InHandle))
    {
        chain::Log(TEXT("Chain [{}] set-distance failed for non-member link [{}]"), InHandle, Link);
        return ECk_Request_OperationResult::Failed;
    }
    Link.Get<FFragment_ChainLink>()._DistanceFromHeadCm = InRequest.Get_DistanceFromHeadCm();
    ck_chain_processor::DoSortRoster(InCurrent._Links);
    ck_chain_processor::DoReserveHistory(InCurrent._History, InCurrent._Links, InParams);
    InHandle.AddOrGet<FTag_Chain_RosterDirty>();
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Chain_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        const FCk_Request_Chain_ReseedHistory& InRequest)
    -> ECk_Request_OperationResult
{
    if (InParams.Get_Solver() == ECk_Chain_Solver::PathHistory)
    {
        InCurrent._History.Reseed(UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InCurrent.Get_Head()), InParams.Get_SampleSpacingCm());
        InHandle.AddOrGet<FTag_Chain_RosterDirty>();
    }
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Chain_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        const FCk_Request_Chain_EnableDisable& InRequest)
    -> ECk_Request_OperationResult
{
    switch (InRequest.Get_EnableDisable())
    {
        case ECk_EnableDisable::Enable:
        {
            InHandle.Try_Remove<FTag_Chain_Disabled>();
            InHandle.AddOrGet<FTag_Chain_RosterDirty>();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InHandle.AddOrGet<FTag_Chain_Disabled>();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InRequest.Get_EnableDisable());
            return ECk_Request_OperationResult::Failed;
        }
    }
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Chain_HandleRequests::
    DoHandleRequest(
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent,
        const FCk_Request_Chain_Split& InRequest)
    -> ECk_Request_OperationResult
{
    const auto HeadPose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InCurrent.Get_Head());
    auto NewChain = InRequest.Get_NewChain();
    const auto IsNewChainValid = ck::IsValid(NewChain) && UCk_Utils_Chain_UE::Has(NewChain)
        && NOT NewChain.Has<FTag_DestroyEntity_Initiate>();
    CK_ENSURE_IF_NOT(IsNewChainValid, TEXT("Chain [{}] split has an invalid new chain"), InHandle)
    { return ECk_Request_OperationResult::Failed; }
    const auto AtLink = InRequest.Get_AtLink();
    const auto AtIndex = InCurrent._Links.IndexOfByKey(AtLink);
    if (AtIndex == INDEX_NONE || NOT ck_chain_processor::DoIsLiveLink(AtLink, InHandle))
    {
        chain::Log(TEXT("Chain [{}] split failed because split link is no longer a member"), InHandle);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewChain);
        return ECk_Request_OperationResult::Failed;
    }
    auto& NewCurrent = NewChain.Get<FFragment_Chain>();
    if (NewChain.Has<FTag_Chain_NeedsSetup>())
    {
        FProcessor_Chain_Setup::ForEachEntity(TimeType{}, NewChain,
            NewChain.Get<FFragment_Chain_Params>(), NewCurrent);
    }
    const auto SplitDistance = UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(AtLink);
    const auto SourceLinks = InCurrent._Links;
    const auto MovedCount = SourceLinks.Num() - AtIndex - 1;
    if (InParams.Get_Solver() == ECk_Chain_Solver::PathHistory)
    {
        NewCurrent._History = InCurrent._History.Slice_Behind(chain::Get_LeadingArcDistance(InCurrent._History, HeadPose) - SplitDistance);
        if (NewCurrent._History.Get_NumSamples() == 0)
        {
            NewCurrent._History.Reseed(
                UCk_Utils_Transform_UE::Get_EntityCurrentTransform(NewCurrent.Get_Head()), InParams.Get_SampleSpacingCm());
        }
    }
    InCurrent._Links.SetNum(AtIndex);
    for (auto Index = AtIndex + 1; Index < SourceLinks.Num(); ++Index)
    {
        auto Link = SourceLinks[Index];
        if (NOT ck_chain_processor::DoIsLiveLink(Link, InHandle))
        { continue; }
        auto& LinkState = Link.Get<FFragment_ChainLink>();
        LinkState._DistanceFromHeadCm -= SplitDistance;
        LinkState._Chain = NewChain;
        Link.Try_Remove<FFragment_ChainLink_TargetPose>();
        NewCurrent._Links.Add(Link);
    }
    auto SplitHead = AtLink;
    SplitHead.Try_Remove<FFragment_ChainLink_Params>();
    SplitHead.Try_Remove<FFragment_ChainLink>();
    SplitHead.Try_Remove<FFragment_ChainLink_TargetPose>();
    ck_chain_processor::DoReserveHistory(NewCurrent._History, NewCurrent._Links, InParams);
    InHandle.AddOrGet<FTag_Chain_RosterDirty>();
    NewChain.AddOrGet<FTag_Chain_RosterDirty>();
    for (auto Index = AtIndex; Index < SourceLinks.Num(); ++Index)
    {
        UUtils_Signal_OnChainLinkDetached::Broadcast(InHandle, MakePayload(InHandle,
            FCk_Chain_Payload_LinkDetached{SourceLinks[Index], ECk_Chain_LinkDetachReason::MovedBySplit}));
    }
    UUtils_Signal_OnChainSplit::Broadcast(InHandle, MakePayload(InHandle,
        FCk_Chain_Payload_Split{NewChain, AtIndex, MovedCount}));
    return ECk_Request_OperationResult::Succeeded;
}

auto
    ck::FProcessor_Chain_CancelPendingRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Chain_Requests& InRequests)
    -> void
{
    const auto RequestsCopy = InRequests._Requests;
    InRequests._Requests.Reset();
    algo::ForEachRequest(RequestsCopy, ck::Visitor([&](const auto& InRequest)
    {
        if constexpr (std::is_same_v<std::decay_t<decltype(InRequest)>, FCk_Request_Chain_Split>)
        {
            auto NewChain = InRequest.Get_NewChain();
            if (ck::IsValid(NewChain))
            { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewChain); }
        }
    }), policy::DontResetContainer{});
    request::FireCancelledForPending(InHandle, RequestsCopy);
}

auto
    ck::FProcessor_Chain_EndPlay::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_Chain& InCurrent)
    -> void
{
    const auto Links = InCurrent._Links;
    InCurrent._Links.Reset();
    for (auto Link : Links)
    {
        if (ck::Is_NOT_Valid(Link, ck::IsValid_Policy_IncludePendingKill{}) || NOT UCk_Utils_ChainLink_UE::Has(Link))
        { continue; }
        const auto& Current = Link.Get<FFragment_ChainLink, ck::IsValid_Policy_IncludePendingKill>();
        if (Current.Get_Chain() != InHandle)
        { continue; }
        Link.Try_Remove<FFragment_ChainLink_Params, ck::IsValid_Policy_IncludePendingKill>();
        Link.Try_Remove<FFragment_ChainLink, ck::IsValid_Policy_IncludePendingKill>();
        Link.Try_Remove<FFragment_ChainLink_TargetPose, ck::IsValid_Policy_IncludePendingKill>();
        UUtils_Signal_OnChainLinkDetached::Broadcast(InHandle, MakePayload(InHandle,
            FCk_Chain_Payload_LinkDetached{Link, ECk_Chain_LinkDetachReason::ChainDestroyed}));
    }
}

auto
    ck::FProcessor_ChainLink_EndPlay::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_ChainLink& InCurrent)
    -> void
{
    auto Chain = InCurrent.Get_Chain();
    if (ck::Is_NOT_Valid(Chain) || NOT UCk_Utils_Chain_UE::Has(Chain) || Chain.Has<FTag_DestroyEntity_Initiate>())
    { return; }
    auto& ChainCurrent = Chain.Get<FFragment_Chain>();
    const auto Index = ChainCurrent._Links.IndexOfByKey(InHandle);
    if (Index == INDEX_NONE)
    { return; }
    ChainCurrent._Links.RemoveAt(Index);
    Chain.AddOrGet<FTag_Chain_RosterDirty>();
    UUtils_Signal_OnChainLinkDetached::Broadcast(Chain, MakePayload(Chain,
        FCk_Chain_Payload_LinkDetached{InHandle, ECk_Chain_LinkDetachReason::LinkDestroyed}));
}

auto
    ck::FProcessor_Chain_Update::
    DoPublishPose(
        FCk_Handle_ChainLink InLink,
        const FFragment_ChainLink_Params& InParams,
        const FTransform& InTargetPose)
    -> void
{
    const auto PathPose = FTransform{InTargetPose.GetRotation(), InTargetPose.GetLocation()};
    auto Target = InParams.Get_LocalOffset() * PathPose;
    Target.SetScale3D(InTargetPose.GetScale3D());
    if (InLink.Has<FFragment_ChainLink_TargetPose>() && Target.Equals(InLink.Get<FFragment_ChainLink_TargetPose>().Get_Pose()))
    { return; }
    auto Transform = UCk_Utils_Transform_UE::CastChecked(InLink);
    UCk_Utils_Transform_UE::Request_SetLocationAndRotation(Transform,
        FCk_Request_Transform_SetLocationAndRotation{Target.GetLocation(), Target.Rotator()}, {});
    InLink.AddOrGet<FFragment_ChainLink_TargetPose>() = FFragment_ChainLink_TargetPose{Target};
}

auto
    ck::FProcessor_Chain_Update::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Chain_Params& InParams,
        FFragment_Chain& InCurrent)
    -> void
{
    const auto IsValidHead = ck::IsValid(InCurrent.Get_Head());
    CK_ENSURE_IF_NOT(IsValidHead, TEXT("Chain [{}] has an invalid head"), InHandle)
    { return; }
    if (InParams.Get_NetPolicy() == ECk_Chain_NetPolicy::AuthorityOnly && NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
    { return; }
    const auto HeadPose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InCurrent.Get_Head());
    ck_chain_processor::DoPruneRoster(InHandle, InCurrent._Links);
    if (InParams.Get_Solver() == ECk_Chain_Solver::PathHistory)
    {
        const auto Chord = FVector::Distance(HeadPose.GetLocation(), InCurrent.Get_LastHeadTransform().GetLocation());
        if (InParams.Get_TeleportDistanceCm() > 0.0f && Chord >= InParams.Get_TeleportDistanceCm())
        {
            const auto From = InCurrent.Get_LastHeadTransform().GetLocation();
            InCurrent._History.Reseed(HeadPose, InParams.Get_SampleSpacingCm());
            InHandle.AddOrGet<FTag_Chain_RosterDirty>();
            chain::Log(TEXT("Chain [{}] head teleported [{}] cm; history reseeded"), InHandle, Chord);
            UUtils_Signal_OnChainHeadTeleported::Broadcast(InHandle, MakePayload(InHandle,
                FCk_Chain_Payload_HeadTeleported{From, HeadPose.GetLocation()}));
        }
        else if (InCurrent._History.Record(HeadPose, InParams.Get_SampleSpacingCm()))
        {
            const auto Length = InCurrent._Links.IsEmpty() ? 0.0f : UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(InCurrent._Links.Last());
            InCurrent._History.Trim(Length + 2.0f * InParams.Get_SampleSpacingCm());
        }
    }
    const auto HeadMoved = NOT HeadPose.Equals(InCurrent.Get_LastHeadTransform());
    InCurrent._LastHeadTransform = HeadPose;
    if (NOT HeadMoved && NOT InHandle.Has<FTag_Chain_RosterDirty>())
    { return; }
    if (InParams.Get_Solver() == ECk_Chain_Solver::PathHistory)
    {
        for (auto Link : InCurrent._Links)
        {
            const auto& Params = Link.Get<FFragment_ChainLink_Params>();
            const auto Distance = UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(Link);
            const auto LinkPose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(Link));
            const auto Pose = chain::Solve_PathHistoryPose(InCurrent._History, HeadPose, Distance, Params.Get_Orientation(),
                InCurrent.Get_UpVectorNormalized(), InParams.Get_HistorySeed(), LinkPose);
            if (Pose.IsSet())
            { DoPublishPose(Link, Params, Pose.GetValue()); }
        }
    }
    else
    {
        auto Poses = TArray<FTransform>{};
        auto Lengths = TArray<float>{};
        auto Orientations = TArray<ECk_Chain_LinkOrientation>{};
        Poses.Reserve(InCurrent._Links.Num());
        Lengths.Reserve(InCurrent._Links.Num());
        Orientations.Reserve(InCurrent._Links.Num());
        auto PreviousDistance = 0.0f;
        for (auto Link : InCurrent._Links)
        {
            const auto Distance = UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(Link);
            Poses.Add(UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(Link)));
            Lengths.Add(Distance - PreviousDistance);
            Orientations.Add(Link.Get<FFragment_ChainLink_Params>().Get_Orientation());
            PreviousDistance = Distance;
        }
        chain::Solve_DistanceConstraint(HeadPose, Lengths, Orientations, InCurrent.Get_UpVectorNormalized(), Poses);
        for (auto Index = 0; Index < InCurrent._Links.Num(); ++Index)
        {
            auto Link = InCurrent._Links[Index];
            DoPublishPose(Link, Link.Get<FFragment_ChainLink_Params>(), Poses[Index]);
        }
    }
    InHandle.Try_Remove<FTag_Chain_RosterDirty>();
}
