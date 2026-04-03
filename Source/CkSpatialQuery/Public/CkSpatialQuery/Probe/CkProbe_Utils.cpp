#include "CkProbe_Utils.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"

#include "CkShapes/CkShapes_Utils.h"

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams,
        const FCk_Probe_DebugInfo& InDebugInfo)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(UCk_Utils_Shapes_UE::Has_Any(InHandle),
        TEXT("Cannot Add a Probe to Entity [{}] because it does NOT have any Shape"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ProbeName()),
        TEXT("Cannot Add a Probe to Entity [{}] because it has INVALID Name"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Probe_Params>(InParams);
    InHandle.Add<ck::FFragment_Probe_DebugInfo>(InDebugInfo);
    InHandle.Add<ck::FFragment_Probe_Current>();

    const auto IsLinearCastProbe =
        InParams.Get_MotionQuality() == ECk_MotionQuality::LinearCast &&
        InParams.Get_MotionType() != ECk_MotionType::Static;

    if (IsLinearCastProbe)
    {
        InHandle.Add<ck::FTag_Probe_LinearCast>();
    }

    InHandle.Add<ck::FTag_Probe_NeedsSetup>();

    auto ProbeHandle = Cast(InHandle);

    Request_EnableDisable(ProbeHandle, FCk_Request_Probe_EnableDisable{InParams.Get_StartingState()});

    return ProbeHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Probe_UE, FCk_Handle_Probe,
    ck::FFragment_Probe_Params, ck::FFragment_Probe_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Get_Name(
        const FCk_Handle_Probe& InProbe)
    -> FGameplayTag
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_ProbeName();
}

auto
    UCk_Utils_Probe_UE::
    Get_ResponsePolicy(
        const FCk_Handle_Probe& InProbe)
    -> ECk_ProbeResponse_Policy
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_ResponsePolicy();
}

auto
    UCk_Utils_Probe_UE::
    Get_Filter(
        const FCk_Handle_Probe& InProbe)
    -> FGameplayTagContainer
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_Filter();
}

auto
    UCk_Utils_Probe_UE::
    Get_ContextOverlapPolicy(
        const FCk_Handle_Probe& InProbe)
    -> ECk_Probe_ContextOverlapPolicy
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_ContextOverlapPolicy();
}

auto
    UCk_Utils_Probe_UE::
    Get_MotionType(
        const FCk_Handle_Probe& InProbe)
    -> ECk_MotionType
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_MotionType();
}

auto
    UCk_Utils_Probe_UE::
    Get_MotionQuality(
        const FCk_Handle_Probe& InProbe)
    -> ECk_MotionQuality
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_MotionQuality();
}

auto
    UCk_Utils_Probe_UE::
    Get_SurfaceInfo(
        const FCk_Handle_Probe& InProbe)
    -> FCk_Probe_SurfaceInfo
{
    return InProbe.Get<ck::FFragment_Probe_Params>().Get_SurfaceInfo();
}

auto
    UCk_Utils_Probe_UE::
    Get_IsEnabledDisabled(
        const FCk_Handle_Probe& InProbe)
    -> ECk_EnableDisable
{
    if (InProbe.Has<ck::FTag_Probe_Disabled>())
    {
        return ECk_EnableDisable::Disable;
    }

    return ECk_EnableDisable::Enable;
}

auto
    UCk_Utils_Probe_UE::
    Get_IsOverlapping(
        const FCk_Handle_Probe& InProbe)
    -> bool
{
    return InProbe.Has<ck::FTag_Probe_Overlapping>();
}

auto
    UCk_Utils_Probe_UE::
    Get_IsOverlappingWith(
        const FCk_Handle_Probe& InProbe,
        const FCk_Handle& InOtherEntity)
    -> bool
{
    const auto& CurrentOverlaps = InProbe.Get<ck::FFragment_Probe_Current>().Get_CurrentOverlaps();

    if (CurrentOverlaps.Contains(FCk_Probe_OverlapInfo{InOtherEntity}))
    { return true; }

    const auto OtherContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InOtherEntity);

    return CurrentOverlaps.Contains(FCk_Probe_OverlapInfo{OtherContextOwner});
}

auto
    UCk_Utils_Probe_UE::
    Get_CanOverlapWith(
        const FCk_Handle_Probe& InA,
        const FCk_Handle_Probe& InB)
    -> bool
{
    if (Get_ResponsePolicy(InA) == ECk_ProbeResponse_Policy::Silent)
    { return false; }

    if (NOT ShouldOverlapWith_ByContext(InA, InB))
    { return false; }

    const auto& Filter = Get_Filter(InA);

    if (Filter.IsEmpty())
    { return true; }

    const auto& Name = Get_Name(InB);

    return Name.MatchesAny(Filter);
}

auto
    UCk_Utils_Probe_UE::
    Get_CurrentOverlaps(
        const FCk_Handle_Probe& InProbe)
    -> TSet<FCk_Probe_OverlapInfo>
{
    return InProbe.Get<ck::FFragment_Probe_Current>().Get_CurrentOverlaps();
}

auto
    UCk_Utils_Probe_UE::
    Request_EnableDisableDebugDraw(
        FCk_Handle_Probe& InProbe,
        ECk_EnableDisable InEnableDisable)
    -> FCk_Handle_Probe
{
    switch(InEnableDisable)
    {
        case ECk_EnableDisable::Enable:
        {
            InProbe.AddOrGet<ck::FTag_Probe_DebugDraw>();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InProbe.Try_Remove<ck::FTag_Probe_DebugDraw>();
            break;
        }
    }

    return InProbe;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Request_BeginOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_BeginOverlap& InRequest)
    -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_OverlapUpdated(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_OverlapUpdated& InRequest)
    -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EndOverlap(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EndOverlap& InRequest)
    -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    Request_EnableDisable(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EnableDisable& InRequest)
    -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>().Update_Requests([&](auto& InContainer)
    {
        InContainer.Emplace(InRequest);
    });

    return InProbe;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Bind to OnBeginOverlap for Probe [{}] because its Response Policy is NOT Notify"), InProbeEntity)
    { return InProbeEntity; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeBeginOverlap, InProbeEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Unbind from OnBeginOverlap for Probe [{}] because its Response Policy is NOT Notify"),
        InProbeEntity)
    { return InProbeEntity; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeBeginOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnOverlapUpdated(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnOverlapUpdated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Bind to OnOverlapUpdated for Probe [{}] because its Response Policy is NOT Notify"),
        InProbeEntity)
    { return InProbeEntity; }

    InProbeEntity.AddOrGet<ck::FTag_Probe_PersistContacts>();

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeOverlapUpdated, InProbeEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnOverlapUpdated(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnOverlapUpdated& InDelegate)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Unbind from OnOverlapUpdated for Probe [{}] because its Response Policy is NOT Notify"),
        InProbeEntity)
    { return InProbeEntity; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeOverlapUpdated, InProbeEntity, InDelegate);

    if (NOT ck::UUtils_Signal_OnProbeOverlapUpdated::IsBoundToDelegate(InProbeEntity))
    {
        InProbeEntity.Try_Remove<ck::FTag_Probe_PersistContacts>();
    }

    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    BindTo_OnEndOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Bind to OnEndOverlap for Probe [{}] because its Response Policy is NOT Notify"), InProbeEntity)
    { return InProbeEntity; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate)
    -> FCk_Handle_Probe
{
    CK_ENSURE_IF_NOT(Get_ResponsePolicy(InProbeEntity) == ECk_ProbeResponse_Policy::Notify,
        TEXT("Cannot Unbind from OnEndOverlap for Probe [{}] because its Response Policy is NOT Notify"), InProbeEntity)
    { return InProbeEntity; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeEndOverlap, InProbeEntity, InDelegate);
    return InProbeEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    BindTo_OnEnableDisable(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEnableDisable& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Probe
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeEnableDisable, InProbeEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    UnbindFrom_OnEnableDisable(
        FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEnableDisable& InDelegate)
    -> FCk_Handle_Probe
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeEnableDisable, InProbeEntity, InDelegate);
    return InProbeEntity;
}

auto
    UCk_Utils_Probe_UE::
    ShouldOverlapWith_ByContext(
        const FCk_Handle_Probe& InProbeA,
        const FCk_Handle_Probe& InProbeB)
    -> bool
{
    if (ck::Is_NOT_Valid(InProbeA) || ck::Is_NOT_Valid(InProbeB))
    { return false; }

    const auto ContextA = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InProbeA);
    const auto ContextB = UCk_Utils_ContextOwner_UE::Get_ContextOwner(InProbeB);
    const auto Policy = Get_ContextOverlapPolicy(InProbeA);

    const auto IsSameContext = (ContextA == ContextB);

    switch (Policy)
    {
        case ECk_Probe_ContextOverlapPolicy::DifferentContextOnly:
        {
            return NOT IsSameContext;
        }
        case ECk_Probe_ContextOverlapPolicy::SameContextOnly:
        {
            return IsSameContext;
        }
        case ECk_Probe_ContextOverlapPolicy::Any:
        {
            return true;
        }
        default:
        {
            CK_INVALID_ENUM(Policy);
            return false;
        }
    }
}

auto
    UCk_Utils_Probe_UE::
    Request_MarkProbe_AsOverlapping(
        FCk_Handle_Probe& InProbeEntity)
    -> void
{
    InProbeEntity.AddOrGet<ck::FTag_Probe_Overlapping>();
}

auto
    UCk_Utils_Probe_UE::
    Request_MarkProbe_AsNotOverlapping(
        FCk_Handle_Probe& InProbeEntity)
    -> void
{
    InProbeEntity.Remove<ck::FTag_Probe_Overlapping>();
}

// --------------------------------------------------------------------------------------------------------------------