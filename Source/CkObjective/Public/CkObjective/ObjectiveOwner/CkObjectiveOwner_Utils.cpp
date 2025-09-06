#include "CkObjectiveOwner_Utils.h"
#include "CkObjectiveOwner_Fragment.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkObjective/Objective/CkObjective_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectiveOwner_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_ObjectiveOwner_ParamsData& InParams)
    -> FCk_Handle_ObjectiveOwner
{
    InHandle.Add<ck::FFragment_ObjectiveOwner_Params>(InParams);
    InHandle.Add<ck::FTag_ObjectiveOwner_NeedsSetup>();
    RecordOfObjectives_Utils::AddIfMissing(InHandle);

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ObjectiveOwner_UE, FCk_Handle_ObjectiveOwner, ck::FFragment_ObjectiveOwner_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectiveOwner_UE::
    Request_AddObjective(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Request_ObjectiveOwner_AddObjective& InRequest)
    -> FCk_Handle_ObjectiveOwner
{
    InOwner.AddOrGet<ck::FFragment_ObjectiveOwner_Requests>()._Requests.Emplace(InRequest);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    Request_RemoveObjective(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Request_ObjectiveOwner_RemoveObjective& InRequest)
    -> FCk_Handle_ObjectiveOwner
{
    InOwner.AddOrGet<ck::FFragment_ObjectiveOwner_Requests>()._Requests.Emplace(InRequest);
    return InOwner;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectiveOwner_UE::
    TryGet_Objective(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        FGameplayTag InObjectiveName)
    -> FCk_Handle_Objective
{
    return RecordOfObjectives_Utils::Get_ValidEntry_ByTag(InObjectiveOwner, InObjectiveName);
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    ForEach_Objective(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner)
    -> TArray<FCk_Handle_Objective>
{
    auto Ret = TArray<FCk_Handle_Objective>{};

    ForEach_Objective(InObjectiveOwner, [&](FCk_Handle_Objective InObjective)
    {
        Ret.Emplace(InObjective);
    });

    return Ret;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    ForEach_Objective(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        const TFunction<void(FCk_Handle_Objective)>& InFunc)
    -> void
{
    RecordOfObjectives_Utils::ForEach_ValidEntry(InObjectiveOwner, InFunc);
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    ForEach_Objective_WithStatus(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        ECk_ObjectiveStatus InObjectiveStatus)
    -> TArray<FCk_Handle_Objective>
{
    auto Ret = TArray<FCk_Handle_Objective>{};

    ForEach_Objective_WithStatus(InObjectiveOwner, InObjectiveStatus, [&](FCk_Handle_Objective InObjective)
    {
        Ret.Emplace(InObjective);
    });

    return Ret;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    ForEach_Objective_WithStatus(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        ECk_ObjectiveStatus InObjectiveStatus,
        const TFunction<void(FCk_Handle_Objective)>& InFunc)
    -> void
{
    RecordOfObjectives_Utils::ForEach_ValidEntry_If(InObjectiveOwner, InFunc, [&](const FCk_Handle_Objective& InObjective)
    {
        return UCk_Utils_Objective_UE::Get_Status(InObjective) == InObjectiveStatus;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectiveOwner_UE::
    BindTo_OnObjectiveAdded(
        FCk_Handle_ObjectiveOwner& InOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ObjectiveOwner_ObjectiveAdded& InDelegate)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveAdded, InOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    BindTo_OnObjectiveRemoved(
        FCk_Handle_ObjectiveOwner& InOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ObjectiveOwner_ObjectiveRemoved& InDelegate)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveRemoved, InOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    BindTo_OnObjectiveStatusChanged(
        FCk_Handle_ObjectiveOwner& InOwner,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ObjectiveOwner_ObjectiveStatusChanged& InDelegate)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveStatusChanged, InOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    UnbindFrom_OnObjectiveAdded(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveAdded& InDelegate)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveAdded, InOwner, InDelegate);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    UnbindFrom_OnObjectiveRemoved(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveRemoved& InDelegate)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveRemoved, InOwner, InDelegate);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    UnbindFrom_OnObjectiveStatusChanged(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveStatusChanged& InDelegate)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveStatusChanged, InOwner, InDelegate);
    return InOwner;
}

// --------------------------------------------------------------------------------------------------------------------