#include "CkProceduralAnimation/Leg/CkProceduralLeg_Utils.h"

#include "CkProceduralAnimation/Gait/CkProceduralGait_Utils.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProceduralLeg_UE, FCk_Handle_ProceduralLeg, ck::FFragment_ProceduralLeg);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralLeg_UE::
    Create(
        FCk_Handle_Transform& InBody,
        const FCk_ProceduralLeg_Spec& InParams)
    -> FCk_Handle_ProceduralLeg
{
    const auto BodyValid = ck::IsValid(InBody)
        && NOT InBody.Has<ck::FTag_DestroyEntity_Initiate>()
        && UCk_Utils_EntityLifetime_UE::Get_CanCreateEntity(InBody)
        && NOT UCk_Utils_ProceduralGait_UE::Has(InBody);
    CK_ENSURE_IF_NOT(BodyValid,
        TEXT("Procedural leg Create rejected body [{}]: it must be a live transform entity that can own children and has no gait yet."),
        InBody)
    { return {}; }

    const auto ParamsValid = ck::IsValid(InParams);
    CK_ENSURE_IF_NOT(ParamsValid,
        TEXT("Procedural leg Create rejected leg [{}] on body [{}]: it needs an Id, a finite placement and 1..8 positive segment lengths."),
        InParams.Get_Id(), InBody)
    { return {}; }

    const auto TopologyValid = Get_Legs(InBody).Num() < 64 && ck::Is_NOT_Valid(TryGet_Leg(InBody, InParams.Get_Id()));
    CK_ENSURE_IF_NOT(TopologyValid,
        TEXT("Procedural leg Create rejected leg [{}] on body [{}]: the Id is already used or the body already has 64 legs."),
        InParams.Get_Id(), InBody)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InBody, [&](FCk_Handle InNewEntity)
    {
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        UCk_Utils_Handle_UE::Set_DebugName(InNewEntity, InParams.Get_Id());
#endif

        InNewEntity.Add<ck::FFragment_ProceduralLeg_Params>(InParams);
        InNewEntity.Add<ck::FFragment_ProceduralLeg>();
    });

    const auto IsNewLegEntityValid = ck::IsValid(NewEntity);
    CK_ENSURE_IF_NOT(IsNewLegEntityValid,
        TEXT("Procedural leg Create could not create a leg entity for body [{}]."), InBody)
    { return {}; }

    auto NewLeg = CastChecked(NewEntity);

    ck::FUtils_RecordOfProceduralLegs::AddIfMissing(InBody, ECk_Record_EntryHandlingPolicy::Default);
    ck::FUtils_RecordOfProceduralLegs::Request_Connect(InBody, NewLeg, ECk_Record_LabelRequirementPolicy::Optional);

    return NewLeg;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralLeg_UE::
    Get_Id(
        const FCk_Handle_ProceduralLeg& InLeg)
    -> FName
{
    return InLeg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Id();
}

auto
    UCk_Utils_ProceduralLeg_UE::
    Get_Placement(
        const FCk_Handle_ProceduralLeg& InLeg)
    -> FCk_ProceduralLeg_Placement
{
    return InLeg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Placement();
}

auto
    UCk_Utils_ProceduralLeg_UE::
    Get_ChainGeometry(
        const FCk_Handle_ProceduralLeg& InLeg)
    -> FCk_ProceduralLeg_ChainGeometry
{
    return InLeg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Chain();
}

auto
    UCk_Utils_ProceduralLeg_UE::
    Get_Foot(
        const FCk_Handle_ProceduralLeg& InLeg)
    -> FCk_ProceduralLeg_Foot
{
    return InLeg.Get<ck::FFragment_ProceduralLeg>().Get_Foot();
}

auto
    UCk_Utils_ProceduralLeg_UE::
    Get_EnableDisable(
        const FCk_Handle_ProceduralLeg& InLeg)
    -> ECk_EnableDisable
{
    return InLeg.Has<ck::FTag_ProceduralLeg_Disabled>() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralLeg_UE::
    Request_EnableDisable(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Request_ProceduralLeg_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_ProceduralLeg
{
    auto Request = InRequest;
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    const auto RequestValid = ck::IsValid(InLeg) && Has(InLeg) && NOT InLeg.Has<ck::FTag_DestroyEntity_Initiate>();
    CK_ENSURE_IF_NOT(RequestValid,
        TEXT("Procedural leg Request_EnableDisable rejected leg [{}]: it must be a live leg entity."), InLeg)
    {
        Request.TryFireCompletion(InLeg, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InLeg;
    }

    InLeg.AddOrGet<ck::FFragment_ProceduralLeg_Requests>()._Requests.Emplace(MoveTemp(Request));

    return InLeg;
}

auto
    UCk_Utils_ProceduralLeg_UE::
    Request_Detach(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Request_ProceduralLeg_Detach& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_ProceduralLeg
{
    auto Request = InRequest;
    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    const auto RequestValid = ck::IsValid(InLeg) && Has(InLeg) && NOT InLeg.Has<ck::FTag_DestroyEntity_Initiate>();
    CK_ENSURE_IF_NOT(RequestValid,
        TEXT("Procedural leg Request_Detach rejected leg [{}]: it must be a live leg entity."), InLeg)
    {
        Request.TryFireCompletion(InLeg, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InLeg;
    }

    InLeg.AddOrGet<ck::FFragment_ProceduralLeg_Requests>()._Requests.Emplace(MoveTemp(Request));

    return InLeg;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralLeg_UE::
    BindTo_OnDetached(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnDetached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProceduralLeg
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProceduralLeg_Detached, InLeg, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InLeg;
}

auto
    UCk_Utils_ProceduralLeg_UE::
    UnbindFrom_OnDetached(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnDetached& InDelegate)
    -> FCk_Handle_ProceduralLeg
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProceduralLeg_Detached, InLeg, InDelegate);
    return InLeg;
}

auto
    UCk_Utils_ProceduralLeg_UE::
    BindTo_OnPlanted(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnPlanted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProceduralLeg
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProceduralLeg_Planted, InLeg, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InLeg;
}

auto
    UCk_Utils_ProceduralLeg_UE::
    UnbindFrom_OnPlanted(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnPlanted& InDelegate)
    -> FCk_Handle_ProceduralLeg
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProceduralLeg_Planted, InLeg, InDelegate);
    return InLeg;
}

auto
    UCk_Utils_ProceduralLeg_UE::
    BindTo_OnLifted(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnLifted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProceduralLeg
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProceduralLeg_Lifted, InLeg, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InLeg;
}

auto
    UCk_Utils_ProceduralLeg_UE::
    UnbindFrom_OnLifted(
        FCk_Handle_ProceduralLeg& InLeg,
        const FCk_Delegate_ProceduralLeg_OnLifted& InDelegate)
    -> FCk_Handle_ProceduralLeg
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProceduralLeg_Lifted, InLeg, InDelegate);
    return InLeg;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProceduralLeg_UE::
    Get_Legs(
        const FCk_Handle& InBody)
    -> TArray<FCk_Handle_ProceduralLeg>
{
    return ck::FUtils_RecordOfProceduralLegs::Get_ValidEntries(InBody);
}

auto
    UCk_Utils_ProceduralLeg_UE::
    TryGet_Leg(
        const FCk_Handle& InBody,
        FName InId)
    -> FCk_Handle_ProceduralLeg
{
    return ck::FUtils_RecordOfProceduralLegs::Get_ValidEntry_If(InBody, [&](const FCk_Handle& InEntry) -> bool
    {
        return InEntry.Get<ck::FFragment_ProceduralLeg_Params>().Get_Id() == InId;
    });
}

// --------------------------------------------------------------------------------------------------------------------
