#include "CkObjective_Utils.h"
#include "CkObjective_Fragment.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Helper function to convert status enum to byte value
    constexpr uint8 StatusEnumToByte(ECk_ObjectiveStatus InStatus)
    {
        return static_cast<uint8>(InStatus);
    }

    // Helper function to convert byte value to status enum
    constexpr ECk_ObjectiveStatus ByteToStatusEnum(uint8 InValue)
    {
        return static_cast<ECk_ObjectiveStatus>(InValue);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Objective_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Objective_ParamsData& InParams)
    -> FCk_Handle_Objective
{
    InHandle.Add<ck::FFragment_Objective_Params>(InParams);
    auto& Current = InHandle.Add<ck::FFragment_Objective_Current>();

    const auto StatusAttributeParams = FCk_Fragment_ByteAttribute_ParamsData{TAG_ByteAttribute_Objective_Status, StatusEnumToByte(ECk_ObjectiveStatus::NotStarted)};
    Current._StatusAttribute = UCk_Utils_ByteAttribute_UE::Add(InHandle, StatusAttributeParams, ECk_Replication::Replicates);

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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Objective_UE::
    Get_Status(
        const FCk_Handle_Objective& InObjective)
    -> ECk_ObjectiveStatus
{
    const auto& Current = InObjective.Get<ck::FFragment_Objective_Current>();

    // Get the status from the byte attribute and convert to enum
    const uint8 StatusValue = UCk_Utils_ByteAttribute_UE::Get_FinalValue(
        Current.Get_StatusAttribute(),
        ECk_MinMaxCurrent::Current);

    return ByteToStatusEnum(StatusValue);
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