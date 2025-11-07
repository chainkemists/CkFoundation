#include "CkObjectiveOwner_Utils.h"
#include "CkObjectiveOwner_Fragment.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEntityCollection/CkEntityCollection_Utils.h"
#include "CkObjective/Objective/CkObjective_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_EntityCollection_Objectives, TEXT("EntityCollection.Objectives"));

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectiveOwner_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_ObjectiveOwner_ParamsData& InParams)
    -> FCk_Handle_ObjectiveOwner
{
    auto ObjectivesCollectionHandle = UCk_Utils_EntityCollection_UE::Add(InHandle,
        FCk_Fragment_EntityCollection_ParamsData{TAG_Label_EntityCollection_Objectives}, ECk_Replication::Replicates);

    InHandle.Add<ck::FFragment_ObjectiveOwner_Params>(InParams);
    InHandle.Add<ck::FFragment_ObjectiveOwner_Current>(ObjectivesCollectionHandle);
    InHandle.Add<ck::FTag_ObjectiveOwner_NeedsSetup>();

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
    const auto& CollectionHandle = InObjectiveOwner.Get<ck::FFragment_ObjectiveOwner_Current>().Get_ObjectivesEntityCollection();
    const auto& Objective = UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils::Get_ValidEntry_ByTag(CollectionHandle, InObjectiveName);
    return UCk_Utils_Objective_UE::Cast(Objective);
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
    const auto& TypeUnsafeFunc = [InFunc](const FCk_Handle& InEntity)
    {
        InFunc(UCk_Utils_Objective_UE::Cast(InEntity));
    };

    const auto& CollectionHandle = InObjectiveOwner.Get<ck::FFragment_ObjectiveOwner_Current>().Get_ObjectivesEntityCollection();
    UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils::ForEach_ValidEntry(CollectionHandle, TypeUnsafeFunc);
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    ForEach_Objective_WithStatus(
        const FCk_Handle_ObjectiveOwner& InObjectiveOwner,
        ECk_ObjectiveStatus InObjectiveStatus)
    -> TArray<FCk_Handle_Objective>
{
    auto Ret = TArray<FCk_Handle_Objective>{};

    ForEach_Objective_WithStatus(InObjectiveOwner, InObjectiveStatus, [&](const FCk_Handle& InObjective)
    {
        Ret.Emplace(UCk_Utils_Objective_UE::Cast(InObjective));
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
    const auto& TypeUnsafeFunc = [InFunc](const FCk_Handle& InEntity)
    {
        InFunc(UCk_Utils_Objective_UE::Cast(InEntity));
    };

    const auto& CollectionHandle = InObjectiveOwner.Get<ck::FFragment_ObjectiveOwner_Current>().Get_ObjectivesEntityCollection();
    UCk_Utils_EntityCollection_UE::EntityCollections_RecordOfEntities_Utils::ForEach_ValidEntry_If(CollectionHandle, TypeUnsafeFunc, [&](const FCk_Handle& InObjective)
    {
        return UCk_Utils_Objective_UE::Get_Status(UCk_Utils_Objective_UE::Cast(InObjective)) == InObjectiveStatus;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ObjectiveOwner_UE::
    BindTo_OnObjectiveAdded(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveAdded& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveAdded, InOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    BindTo_OnObjectiveRemoved(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ObjectiveOwner
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnObjectiveOwner_ObjectiveRemoved, InOwner, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InOwner;
}

auto
    UCk_Utils_ObjectiveOwner_UE::
    BindTo_OnObjectiveStatusChanged(
        FCk_Handle_ObjectiveOwner& InOwner,
        const FCk_Delegate_ObjectiveOwner_ObjectiveStatusChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
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
