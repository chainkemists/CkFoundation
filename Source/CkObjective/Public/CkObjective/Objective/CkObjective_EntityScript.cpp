#include "CkObjective_EntityScript.h"
#include "CkObjective_Utils.h"
#include "CkObjective_Fragment.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCue/CkCue_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Objective_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Ret = Super::Construct(InHandle, InSpawnParams);

    const auto ObjectiveParams = FCk_Objective_ParamsData{_ObjectiveName}
        .Set_DisplayName(_DisplayName)
        .Set_Description(_Description);

    auto ObjectiveHandle = UCk_Utils_Objective_UE::Add(_AssociatedEntity, ObjectiveParams);

    auto StatusDelegate = FCk_Delegate_Objective_StatusChanged{};
    StatusDelegate.BindDynamic(this, &ThisType::HandleStatusChanged);
    UCk_Utils_Objective_UE::BindTo_OnStatusChanged(ObjectiveHandle,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior::DoNothing,
        StatusDelegate);

    auto ProgressDelegate = FCk_Delegate_Objective_ProgressChanged{};
    ProgressDelegate.BindDynamic(this, &ThisType::HandleProgressChanged);
    UCk_Utils_Objective_UE::BindTo_OnProgressChanged(ObjectiveHandle,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior::DoNothing,
        ProgressDelegate);

    auto CompletedDelegate = FCk_Delegate_Objective_Completed{};
    CompletedDelegate.BindDynamic(this, &ThisType::HandleObjectiveCompleted);
    UCk_Utils_Objective_UE::BindTo_OnCompleted(ObjectiveHandle,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior::DoNothing,
        CompletedDelegate);

    auto FailedDelegate = FCk_Delegate_Objective_Failed{};
    FailedDelegate.BindDynamic(this, &ThisType::HandleObjectiveFailed);
    UCk_Utils_Objective_UE::BindTo_OnFailed(ObjectiveHandle,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior::DoNothing,
        FailedDelegate);

    return Ret;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Objective_EntityScript::
    StartObjective()
    -> void
{
    auto ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    UCk_Utils_Objective_UE::Request_Start(ObjectiveHandle, FCk_Request_Objective_Start{});
}

auto
    UCk_Objective_EntityScript::
    CompleteObjective(
        FGameplayTag InMetaData)
    -> void
{
    auto ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    UCk_Utils_Objective_UE::Request_Complete(ObjectiveHandle, FCk_Request_Objective_Complete{}.Set_MetaData(InMetaData));
}

auto
    UCk_Objective_EntityScript::
    FailObjective(
        FGameplayTag InMetaData)
    -> void
{
    auto ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    UCk_Utils_Objective_UE::Request_Fail(ObjectiveHandle, FCk_Request_Objective_Fail{}.Set_MetaData(InMetaData));
}

auto
    UCk_Objective_EntityScript::
    UpdateProgress(
        int32 InNewProgress,
        FGameplayTag InMetaData)
    -> void
{
    auto ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    UCk_Utils_Objective_UE::Request_UpdateProgress(ObjectiveHandle, FCk_Request_Objective_UpdateProgress{InNewProgress}.Set_MetaData(InMetaData));
}

auto
    UCk_Objective_EntityScript::
    AddProgress(
        int32 InProgressDelta,
        FGameplayTag InMetaData)
    -> void
{
    auto ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    UCk_Utils_Objective_UE::Request_AddProgress(ObjectiveHandle, FCk_Request_Objective_AddProgress{InProgressDelta}.Set_MetaData(InMetaData));
}

auto
    UCk_Objective_EntityScript::
    Get_CurrentStatus() const
    -> ECk_ObjectiveStatus
{
    const auto& ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    return UCk_Utils_Objective_UE::Get_Status(ObjectiveHandle);
}

auto
    UCk_Objective_EntityScript::
    Get_CurrentProgress() const
    -> int32
{
    const auto& ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    return UCk_Utils_Objective_UE::Get_Progress(ObjectiveHandle);
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Objective_EntityScript::
    HandleStatusChanged(
        FCk_Handle_Objective InObjective,
        ECk_ObjectiveStatus NewStatus)
{
    switch (NewStatus)
    {
        case ECk_ObjectiveStatus::Active:
        {
            OnObjectiveStarted(InObjective);
            break;
        }
        default:
        {
            break;
        }
    }
}

auto
    UCk_Objective_EntityScript::
    HandleProgressChanged(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData,
        int32 OldProgress,
        int32 NewProgress)
    -> void
{
    OnProgressChanged(InObjective, InMetaData, OldProgress, NewProgress);
}

void
    UCk_Objective_EntityScript::
    HandleObjectiveCompleted(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData)
{
    OnObjectiveCompleted(InObjective, InMetaData);
}

void
    UCk_Objective_EntityScript::
    HandleObjectiveFailed(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData)
{
    OnObjectiveFailed(InObjective, InMetaData);
}

// --------------------------------------------------------------------------------------------------------------------