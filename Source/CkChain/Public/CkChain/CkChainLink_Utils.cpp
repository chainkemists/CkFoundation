#include "CkChainLink_Utils.h"

#include "CkChain/CkChain_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ChainLink_UE, FCk_Handle_ChainLink, ck::FFragment_ChainLink, ck::FFragment_ChainLink_Params);

auto
    UCk_Utils_ChainLink_UE::
    Get_Chain(
        const FCk_Handle_ChainLink& InLink)
    -> FCk_Handle_Chain
{
    return InLink.Get<ck::FFragment_ChainLink>().Get_Chain();
}

auto
    UCk_Utils_ChainLink_UE::
    Get_DistanceFromHeadCm(
        const FCk_Handle_ChainLink& InLink)
    -> float
{
    return InLink.Get<ck::FFragment_ChainLink>().Get_DistanceFromHeadCm();
}

auto
    UCk_Utils_ChainLink_UE::
    Get_LocalOffset(
        const FCk_Handle_ChainLink& InLink)
    -> FTransform
{
    return InLink.Get<ck::FFragment_ChainLink_Params>().Get_LocalOffset();
}

auto
    UCk_Utils_ChainLink_UE::
    Get_Orientation(
        const FCk_Handle_ChainLink& InLink)
    -> ECk_Chain_LinkOrientation
{
    return InLink.Get<ck::FFragment_ChainLink_Params>().Get_Orientation();
}

auto
    UCk_Utils_ChainLink_UE::
    Get_TargetPose(
        const FCk_Handle_ChainLink& InLink)
    -> FTransform
{
    if (InLink.Has<ck::FFragment_ChainLink_TargetPose>())
    { return InLink.Get<ck::FFragment_ChainLink_TargetPose>().Get_Pose(); }

    return UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(InLink));
}

auto
    UCk_Utils_ChainLink_UE::
    Get_HasTargetPose(
        const FCk_Handle_ChainLink& InLink)
    -> bool
{
    return InLink.Has<ck::FFragment_ChainLink_TargetPose>();
}

auto
    UCk_Utils_ChainLink_UE::
    Get_IsHeld(
        const FCk_Handle_ChainLink& InLink)
    -> bool
{
    const auto Chain = Get_Chain(InLink);
    if (ck::Is_NOT_Valid(Chain) || NOT UCk_Utils_Chain_UE::Has(Chain))
    { return false; }

    const auto& Params = Chain.Get<ck::FFragment_Chain_Params>();
    const auto& History = Chain.Get<ck::FFragment_Chain>().Get_History();
    const auto HeadPose = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Chain_UE::Get_Head(Chain));
    return UCk_Utils_Chain_UE::Get_Solver(Chain) == ECk_Chain_Solver::PathHistory &&
        Params.Get_HistorySeed() == ECk_Chain_HistorySeed::HoldUntilCovered &&
        (History.Get_NumSamples() == 0 ||
         ck::chain::Get_LeadingArcDistance(History, HeadPose) - Get_DistanceFromHeadCm(InLink) < History.Get_OldestArcDistance());
}

auto
    UCk_Utils_ChainLink_UE::
    Get_Index(
        const FCk_Handle_ChainLink& InLink)
    -> int32
{
    const auto Chain = Get_Chain(InLink);
    if (ck::Is_NOT_Valid(Chain) || NOT UCk_Utils_Chain_UE::Has(Chain))
    { return INDEX_NONE; }
    return Chain.Get<ck::FFragment_Chain>().Get_Links().IndexOfByKey(InLink);
}
