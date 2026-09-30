#include "CkChain_Utils.h"

#include "CkChain/CkChainLink_Utils.h"
#include "CkChain/CkChain_Log.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Chain_UE, FCk_Handle_Chain, ck::FFragment_Chain, ck::FFragment_Chain_Params);

auto
    UCk_Utils_Chain_UE::
    Add(
        FCk_Handle_Transform& InHead,
        const FCk_Chain_Spec& InParams)
    -> FCk_Handle_Chain
{
    const auto IsValidHead = ck::IsValid(InHead) && UCk_Utils_Transform_UE::Has(InHead);
    CK_ENSURE_IF_NOT(IsValidHead, TEXT("Chain Add rejected invalid head [{}]"), InHead)
    { return {}; }

    const auto IsValidParams = InParams.Get_IsValid();
    CK_ENSURE_IF_NOT(IsValidParams, TEXT("Chain Add rejected invalid parameters"))
    { return {}; }

    const auto CanCreate = UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InHead);
    CK_ENSURE_IF_NOT(CanCreate, TEXT("Chain Add cannot create a child of head [{}]"), InHead)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHead, [&](FCk_Handle InNewEntity)
    {
        if (InParams.Get_ChainName().IsValid())
        { UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_ChainName()); }
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        else
        { UCk_Utils_Handle_UE::Set_DebugName(InNewEntity, "Chain: No Name Specified"); }
#endif

        InNewEntity.Add<ck::FFragment_Chain_Params>(InParams.Get_Solver(), InParams.Get_SampleSpacingCm(), InParams.Get_HistorySeed(),
            InParams.Get_TeleportDistanceCm(), InParams.Get_UpVector().GetSafeNormal(), InParams.Get_NetPolicy());
        if (InParams.Get_StartingState() == ECk_EnableDisable::Disable)
        { InNewEntity.Add<ck::FTag_Chain_Disabled>(); }

        InNewEntity.Add<ck::FFragment_Chain>(InHead);
        InNewEntity.Add<ck::FTag_Chain_NeedsSetup>();
    });
    const auto IsCreated = ck::IsValid(NewEntity);
    CK_ENSURE_IF_NOT(IsCreated, TEXT("Chain Add failed to create child for head [{}]"), InHead)
    { return {}; }
    auto Chain = CastChecked(NewEntity);
    RecordOfChains_Utils::AddIfMissing(InHead, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfChains_Utils::Request_Connect(InHead, Chain, ECk_Record_LabelRequirementPolicy::Optional);
    return Chain;
}

auto
    UCk_Utils_Chain_UE::
    Has_Any(
        const FCk_Handle& InHead)
    -> bool
{
    return RecordOfChains_Utils::Has(InHead);
}

auto
    UCk_Utils_Chain_UE::
    TryGet_Chain(
        const FCk_Handle& InHead,
        FGameplayTag InChainName)
    -> FCk_Handle_Chain
{
    return RecordOfChains_Utils::Get_ValidEntry_ByTag(InHead, InChainName);
}

auto
    UCk_Utils_Chain_UE::
    ForEach_Chain(
        const FCk_Handle& InHead,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Chain>
{
    auto Chains = TArray<FCk_Handle_Chain>{};
    ForEach_Chain(InHead, [&](FCk_Handle_Chain InChain)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InChain, InOptionalPayload); }
        else
        { Chains.Add(InChain); }
    });
    return Chains;
}

auto
    UCk_Utils_Chain_UE::
    ForEach_Chain(
        const FCk_Handle& InHead,
        const TFunction<void(FCk_Handle_Chain)>& InFunc)
    -> void
{
    RecordOfChains_Utils::ForEach_ValidEntry(InHead, InFunc);
}

auto
    UCk_Utils_Chain_UE::
    Get_Head(
        const FCk_Handle_Chain& InChain)
    -> FCk_Handle_Transform
{
    return InChain.Get<ck::FFragment_Chain>().Get_Head();
}

auto
    UCk_Utils_Chain_UE::
    Get_Solver(
        const FCk_Handle_Chain& InChain)
    -> ECk_Chain_Solver
{
    return InChain.Get<ck::FFragment_Chain_Params>().Get_Solver();
}

auto
    UCk_Utils_Chain_UE::
    Get_NetPolicy(
        const FCk_Handle_Chain& InChain)
    -> ECk_Chain_NetPolicy
{
    return InChain.Get<ck::FFragment_Chain_Params>().Get_NetPolicy();
}

auto
    UCk_Utils_Chain_UE::
    Get_IsEnabled(
        const FCk_Handle_Chain& InChain)
    -> bool
{
    return NOT InChain.Has<ck::FTag_Chain_Disabled>();
}

auto
    UCk_Utils_Chain_UE::
    Get_Links(
        const FCk_Handle_Chain& InChain)
    -> TArray<FCk_Handle_ChainLink>
{
    return InChain.Get<ck::FFragment_Chain>().Get_Links();
}

auto
    UCk_Utils_Chain_UE::
    Get_NumLinks(
        const FCk_Handle_Chain& InChain)
    -> int32
{
    return InChain.Get<ck::FFragment_Chain>().Get_Links().Num();
}

auto
    UCk_Utils_Chain_UE::
    Get_LengthCm(
        const FCk_Handle_Chain& InChain)
    -> float
{
    const auto& Links = InChain.Get<ck::FFragment_Chain>().Get_Links();
    return Links.IsEmpty() ? 0.0f : UCk_Utils_ChainLink_UE::Get_DistanceFromHeadCm(Links.Last());
}

auto
    UCk_Utils_Chain_UE::
    Get_HistoryLengthCm(
        const FCk_Handle_Chain& InChain)
    -> float
{
    const auto& History = InChain.Get<ck::FFragment_Chain>().Get_History();
    return History.Get_NumSamples() == 0 ? 0.0f : History.Get_HeadArcDistance() - History.Get_OldestArcDistance();
}

auto
    UCk_Utils_Chain_UE::
    Get_NumHistorySamples(
        const FCk_Handle_Chain& InChain)
    -> int32
{
    return InChain.Get<ck::FFragment_Chain>().Get_History().Get_NumSamples();
}

auto
    UCk_Utils_Chain_UE::
    Get_HistorySamples(
        const FCk_Handle_Chain& InChain)
    -> TArray<FCk_Chain_PathSample>
{
    return InChain.Get<ck::FFragment_Chain>().Get_History().Get_Samples();
}

auto
    UCk_Utils_Chain_UE::
    Get_PoseAtDistance(
        const FCk_Handle_Chain& InChain,
        float InDistanceFromHeadCm)
    -> FCk_Chain_PoseAtDistance_Result
{
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain Get Pose At Distance requires a valid chain [{}]"), InChain)
    { return {}; }

    const auto IsPathHistory = Get_Solver(InChain) == ECk_Chain_Solver::PathHistory;
    CK_ENSURE_IF_NOT(IsPathHistory, TEXT("Chain Get Pose At Distance requires PathHistory solver"))
    { return {}; }

    const auto IsValidDistance = FMath::IsFinite(InDistanceFromHeadCm) && InDistanceFromHeadCm >= 0.0f;
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain Get Pose At Distance requires a finite nonnegative distance"))
    { return {}; }

    const auto& Current = InChain.Get<ck::FFragment_Chain>();
    const auto IsValidHead = ck::IsValid(Current.Get_Head());
    CK_ENSURE_IF_NOT(IsValidHead, TEXT("Chain Get Pose At Distance requires a valid head [{}]"), InChain)
    { return {}; }

    if (Current.Get_History().Get_NumSamples() == 0)
    { return {}; }

    const auto& Params = InChain.Get<ck::FFragment_Chain_Params>();
    const auto HeadPose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(Current.Get_Head());
    const auto Pose = ck::chain::Solve_PathHistoryPose(Current.Get_History(), HeadPose, InDistanceFromHeadCm,
        ECk_Chain_LinkOrientation::FollowPath, Params.Get_UpVectorNormalized(), Params.Get_HistorySeed(), HeadPose);
    if (NOT Pose.IsSet())
    { return {}; }

    constexpr auto IsValidPose = true;
    return FCk_Chain_PoseAtDistance_Result{Pose.GetValue(), IsValidPose};
}

auto
    UCk_Utils_Chain_UE::
    Request_AttachLink(
        FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_AttachLink& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Chain
{
    if (ck::IsValid(InChain) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InChain,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain AttachLink rejected invalid chain [{}]"), InChain)
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidLink = ck::IsValid(InRequest.Get_Link()) && UCk_Utils_Transform_UE::Has(InRequest.Get_Link());
    CK_ENSURE_IF_NOT(IsValidLink, TEXT("Chain AttachLink rejected invalid link"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto& Link = InRequest.Get_Link();
    const auto IsAttachable = Link != Get_Head(InChain) && NOT UCk_Utils_ChainLink_UE::Has(Link)
        && NOT UCk_Utils_SceneNode_UE::Has(Link)
        && (NOT Link.Has<ck::FFragment_Transform_RootComponent>() || Link.Has<ck::FTag_Transform_Movable>())
        && (Get_NetPolicy(InChain) != ECk_Chain_NetPolicy::Everywhere || NOT Link.Has<ck::FFragment_ContainerRef_Location>())
        && InRequest.Get_LinkSpec().Get_IsValid();
    CK_ENSURE_IF_NOT(IsAttachable, TEXT("Chain AttachLink rejected incompatible link or parameters"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Chain_Requests, InChain);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InChain.AddOrGet<ck::FFragment_Chain_Requests>()._Requests.Emplace(Request);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    Request_DetachLink(
        FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_DetachLink& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Chain
{
    if (ck::IsValid(InChain) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InChain,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain DetachLink rejected invalid chain [{}]"), InChain)
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidLink = ck::IsValid(InRequest.Get_Link());
    CK_ENSURE_IF_NOT(IsValidLink, TEXT("Chain DetachLink rejected invalid link"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Chain_Requests, InChain);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InChain.AddOrGet<ck::FFragment_Chain_Requests>()._Requests.Emplace(Request);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    Request_SetLinkDistance(
        FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_SetLinkDistance& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Chain
{
    if (ck::IsValid(InChain) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InChain,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain SetLinkDistance rejected invalid chain [{}]"), InChain)
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidLink = ck::IsValid(InRequest.Get_Link());
    CK_ENSURE_IF_NOT(IsValidLink, TEXT("Chain SetLinkDistance rejected invalid link"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidDistance = FMath::IsFinite(InRequest.Get_DistanceFromHeadCm()) && InRequest.Get_DistanceFromHeadCm() > 0.0f;
    CK_ENSURE_IF_NOT(IsValidDistance, TEXT("Chain SetLinkDistance rejected invalid distance"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Chain_Requests, InChain);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InChain.AddOrGet<ck::FFragment_Chain_Requests>()._Requests.Emplace(Request);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    Request_Split(
        FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_Split& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Chain
{
    if (ck::IsValid(InChain) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InChain,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain Split rejected invalid chain [{}]"), InChain)
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }
    const auto IsValidLink = ck::IsValid(InRequest.Get_AtLink());
    CK_ENSURE_IF_NOT(IsValidLink, TEXT("Chain Split rejected invalid link"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }
    const auto IsMember = InChain.Get<ck::FFragment_Chain>().Get_Links().Contains(InRequest.Get_AtLink());
    CK_ENSURE_IF_NOT(IsMember, TEXT("Chain Split rejected non-member link"))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }
    auto Request = InRequest;
    auto NewHead = UCk_Utils_Transform_UE::CastChecked(InRequest.Get_AtLink());
    Request._NewChain = Add(NewHead, DoGet_Spec(InChain));
    if (ck::Is_NOT_Valid(Request.Get_NewChain()))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    Request._NewChain.Add<ck::FTag_Chain_SplitPending>();
    CK_CALLSTACK_RECORD(ck::FFragment_Chain_Requests, InChain);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InChain.AddOrGet<ck::FFragment_Chain_Requests>()._Requests.Emplace(Request);
    return Request.Get_NewChain();
}

auto
    UCk_Utils_Chain_UE::
    Request_ReseedHistory(
        FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_ReseedHistory& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Chain
{
    if (ck::IsValid(InChain) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InChain,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain ReseedHistory rejected invalid chain [{}]"), InChain)
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Chain_Requests, InChain);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InChain.AddOrGet<ck::FFragment_Chain_Requests>()._Requests.Emplace(Request);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    Request_EnableDisable(
        FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Chain
{
    if (ck::IsValid(InChain) && UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InChain,
        ECk_EntityLifetime_DestructionPhase::BeginDestroy))
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    const auto IsValidChain = ck::IsValid(InChain) && Has(InChain);
    CK_ENSURE_IF_NOT(IsValidChain, TEXT("Chain EnableDisable rejected invalid chain [{}]"), InChain)
    {
        InDelegate.ExecuteIfBound(InChain, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InChain;
    }
    auto Request = InRequest;
    CK_CALLSTACK_RECORD(ck::FFragment_Chain_Requests, InChain);
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }
    InChain.AddOrGet<ck::FFragment_Chain_Requests>()._Requests.Emplace(Request);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    BindTo_OnLinkAttached(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkAttached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnChainLinkAttached, InChain, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    UnbindFrom_OnLinkAttached(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkAttached& InDelegate)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnChainLinkAttached, InChain, InDelegate);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    BindTo_OnLinkDetached(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkDetached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnChainLinkDetached, InChain, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    UnbindFrom_OnLinkDetached(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkDetached& InDelegate)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnChainLinkDetached, InChain, InDelegate);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    BindTo_OnSplit(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnSplit& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnChainSplit, InChain, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    UnbindFrom_OnSplit(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnSplit& InDelegate)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnChainSplit, InChain, InDelegate);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    BindTo_OnHeadTeleported(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnHeadTeleported& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnChainHeadTeleported, InChain, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    UnbindFrom_OnHeadTeleported(
        FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnHeadTeleported& InDelegate)
    -> FCk_Handle_Chain
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnChainHeadTeleported, InChain, InDelegate);
    return InChain;
}

auto
    UCk_Utils_Chain_UE::
    DoGet_Spec(
        const FCk_Handle_Chain& InChain)
    -> FCk_Chain_Spec
{
    const auto& Params = InChain.Get<ck::FFragment_Chain_Params>();
    auto Spec = FCk_Chain_Spec{Params.Get_Solver()};
    Spec.Set_SampleSpacingCm(Params.Get_SampleSpacingCm());
    Spec.Set_TeleportDistanceCm(Params.Get_TeleportDistanceCm());
    Spec.Set_UpVector(Params.Get_UpVectorNormalized());
    Spec.Set_HistorySeed(Params.Get_HistorySeed());
    Spec.Set_NetPolicy(Params.Get_NetPolicy());
    if (UCk_Utils_GameplayLabel_UE::Has(InChain))
    { Spec.Set_ChainName(UCk_Utils_GameplayLabel_UE::Get_Label(InChain)); }

    return Spec;
}
