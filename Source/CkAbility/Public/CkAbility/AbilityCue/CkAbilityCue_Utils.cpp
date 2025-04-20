#pragma once

#include "CkAbilityCue_Utils.h"

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include <Blueprint/BlueprintExceptionInfo.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkAbilityCue"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityCue_UE::
    Request_Spawn_AbilityCue(
        const FCk_Handle& InHandle,
        const FCk_Request_AbilityCue_Spawn& InRequest)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    RequestEntity.AddOrGet<ck::FFragment_AbilityCue_SpawnRequest>(
        InRequest.Get_AbilityCueName(), InRequest.Get_WorldContextObject(), InRequest.Get_ReplicatedParams(), ECk_Replication::Replicates);
}

auto
    UCk_Utils_AbilityCue_UE::
    Request_Spawn_AbilityCue_Local(
        const FCk_Handle& InHandle,
        const FCk_Request_AbilityCue_Spawn& InRequest)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    RequestEntity.AddOrGet<ck::FFragment_AbilityCue_SpawnRequest>(
        InRequest.Get_AbilityCueName(), InRequest.Get_WorldContextObject(), InRequest.Get_ReplicatedParams(), ECk_Replication::DoesNotReplicate);
}

auto
    UCk_Utils_AbilityCue_UE::
    Get_Params(
        const FCk_Handle& InAbilityCueEntity)
    -> FCk_AbilityCue_Params
{
    return InAbilityCueEntity.Get<FCk_AbilityCue_Params>();
}

auto
    UCk_Utils_AbilityCue_UE::
    Make_AbilityCue_Params(
        FVector InLocation,
        FVector InNormal,
        FCk_Handle InInstigator,
        FCk_Handle InEffectCauser)
    -> FCk_AbilityCue_Params
{
    auto AbilityCueParams = FCk_AbilityCue_Params{};

    AbilityCueParams.Set_Location(InLocation);
    AbilityCueParams.Set_Normal(InNormal);

    if (ck::IsValid(InInstigator))
    {
        AbilityCueParams.Set_Instigator(InInstigator);
    }

    if (ck::IsValid(InEffectCauser))
    {
        AbilityCueParams.Set_EffectCauser(InEffectCauser);
    }

    return AbilityCueParams;
}

DEFINE_FUNCTION(UCk_Utils_AbilityCue_UE::execINTERNAL__Make_AbilityCue_Params_With_CustomData)
{
    P_GET_STRUCT(FVector, Location);
    P_GET_STRUCT(FVector, Normal);
    P_GET_STRUCT(FCk_Handle, Instigator);
    P_GET_STRUCT(FCk_Handle, EffectCauser);

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    const void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_SetInvalidValueWarning", "Failed to resolve the Value for Broadcast"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }
    else
    {
        P_NATIVE_BEGIN;
        FInstancedStruct InstancedStruct;
        InstancedStruct.InitializeAs(ValueProp->Struct, static_cast<const uint8*>(ValuePtr));
        auto AbilityCueParams = Make_AbilityCue_Params_With_CustomData(Location, Normal, Instigator, EffectCauser, InstancedStruct);
        *static_cast<decltype(AbilityCueParams)*>(RESULT_PARAM) = AbilityCueParams;
        P_NATIVE_END;
    }
}

auto
    UCk_Utils_AbilityCue_UE::
    Make_AbilityCue_Params_With_CustomData(
        FVector InLocation,
        FVector InNormal,
        FCk_Handle InInstigator,
        FCk_Handle InEffectCauser,
        FInstancedStruct InCustomData)
    -> FCk_AbilityCue_Params
{
    auto AbilityCueParams = Make_AbilityCue_Params(InLocation, InNormal, InInstigator, InEffectCauser);
    AbilityCueParams.Set_CustomData(InCustomData);

    return AbilityCueParams;
}

DEFINE_FUNCTION(UCk_Utils_AbilityCue_UE::execINTERNAL__Make_AbilityCue_With_CustomData)
{
    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    const void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_SetInvalidValueWarning", "Failed to resolve the Value for Broadcast"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }
    else
    {
        P_NATIVE_BEGIN;
        FInstancedStruct InstancedStruct;
        InstancedStruct.InitializeAs(ValueProp->Struct, static_cast<const uint8*>(ValuePtr));
        auto AbilityCueParams = Make_AbilityCue_With_CustomData(InstancedStruct);
        *static_cast<decltype(AbilityCueParams)*>(RESULT_PARAM) = AbilityCueParams;
        P_NATIVE_END;
    }
}

auto
    UCk_Utils_AbilityCue_UE::
    Make_AbilityCue_With_CustomData(
        FInstancedStruct InCustomData)
    -> FCk_AbilityCue_Params
{
    auto AbilityCueParams = FCk_AbilityCue_Params{};
    AbilityCueParams.Set_CustomData(InCustomData);

    return AbilityCueParams;
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_MESSAGE
