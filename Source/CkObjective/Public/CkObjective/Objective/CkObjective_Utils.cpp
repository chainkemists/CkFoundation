#include "CkObjective_Utils.h"
#include "CkObjective_Fragment.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Objective_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Objective_ParamsData& InParams)
    -> FCk_Handle_Objective
{
    InHandle.Add<ck::FFragment_Objective_Params>(InParams);
    InHandle.Add<ck::FFragment_Objective_Current>();
    UCk_Utils_GameplayLabel_UE::Add(InHandle, InParams.Get_ObjectiveName());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Objective_UE, FCk_Handle_Objective,
    ck::FFragment_Objective_Current, ck::FFragment_Objective_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Objective_UE::
    Request_Start(
        FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_Start& InRequest)
    -> FCk_Handle_Objective
{
    InObjective.AddOrGet<ck::FFragment_Objective_Requests>()._Requests.Emplace(InRequest);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    Request_Complete(
        FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_Complete& InRequest)
    -> FCk_Handle_Objective
{
    InObjective.AddOrGet<ck::FFragment_Objective_Requests>()._Requests.Emplace(InRequest);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    Request_Fail(
        FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_Fail& InRequest)
    -> FCk_Handle_Objective
{
    InObjective.AddOrGet<ck::FFragment_Objective_Requests>()._Requests.Emplace(InRequest);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    Request_UpdateProgress(
        FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_UpdateProgress& InRequest)
    -> FCk_Handle_Objective
{
    InObjective.AddOrGet<ck::FFragment_Objective_Requests>()._Requests.Emplace(InRequest);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    Request_AddProgress(
        FCk_Handle_Objective& InObjective,
        const FCk_Request_Objective_AddProgress& InRequest)
    -> FCk_Handle_Objective
{
    InObjective.AddOrGet<ck::FFragment_Objective_Requests>()._Requests.Emplace(InRequest);
    return InObjective;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Objective_UE::
    Get_Status(
        const FCk_Handle_Objective& InObjective)
    -> ECk_ObjectiveStatus
{
    return InObjective.Get<ck::FFragment_Objective_Current>().Get_Status();
}

auto
    UCk_Utils_Objective_UE::
    Get_Progress(
        const FCk_Handle_Objective& InObjective)
    -> int32
{
    return InObjective.Get<ck::FFragment_Objective_Current>().Get_Progress();
}

auto
    UCk_Utils_Objective_UE::
    Get_Name(
        const FCk_Handle_Objective& InObjective)
    -> FGameplayTag
{
    return InObjective.Get<ck::FFragment_Objective_Params>().Get_ObjectiveName();
}

auto
    UCk_Utils_Objective_UE::
    Get_DisplayName(
        const FCk_Handle_Objective& InObjective)
    -> FText
{
    return InObjective.Get<ck::FFragment_Objective_Params>().Get_DisplayName();
}

auto
    UCk_Utils_Objective_UE::
    Get_Description(
        const FCk_Handle_Objective& InObjective)
    -> FText
{
    return InObjective.Get<ck::FFragment_Objective_Params>().Get_Description();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Objective_UE::
    BindTo_OnStatusChanged(
        FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_StatusChanged& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjective_StatusChanged, InObjective, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    BindTo_OnProgressChanged(
        FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_ProgressChanged& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjective_ProgressChanged, InObjective, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    BindTo_OnCompleted(
        FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_Completed& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjective_Completed, InObjective, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    BindTo_OnFailed(
        FCk_Handle_Objective& InObjective,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Objective_Failed& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjective_Failed, InObjective, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    UnbindFrom_OnStatusChanged(
        FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_StatusChanged& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjective_StatusChanged, InObjective, InDelegate);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    UnbindFrom_OnProgressChanged(
        FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_ProgressChanged& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjective_ProgressChanged, InObjective, InDelegate);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    UnbindFrom_OnCompleted(
        FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_Completed& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjective_Completed, InObjective, InDelegate);
    return InObjective;
}

auto
    UCk_Utils_Objective_UE::
    UnbindFrom_OnFailed(
        FCk_Handle_Objective& InObjective,
        const FCk_Delegate_Objective_Failed& InDelegate)
    -> FCk_Handle_Objective
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnObjective_Failed, InObjective, InDelegate);
    return InObjective;
}

// --------------------------------------------------------------------------------------------------------------------