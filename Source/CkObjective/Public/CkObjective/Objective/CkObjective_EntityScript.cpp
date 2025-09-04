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

    return Ret;
}

auto
    UCk_Objective_EntityScript::
    BeginPlay()
    -> void
{
    auto ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);

    auto StatusDelegate = FCk_Delegate_Objective_StatusChanged{};
    StatusDelegate.BindDynamic(this, &ThisType::HandleStatusChanged);
    UCk_Utils_Objective_UE::BindTo_OnStatusChanged(ObjectiveHandle,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior::DoNothing,
        StatusDelegate);

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

    Super::BeginPlay();
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
    Get_CurrentStatus() const
    -> ECk_ObjectiveStatus
{
    const auto& ObjectiveHandle = UCk_Utils_Objective_UE::Cast(_AssociatedEntity);
    return UCk_Utils_Objective_UE::Get_Status(ObjectiveHandle);
}


// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Objective_EntityScript::
    HandleStatusChanged(
        FCk_Handle_Objective InObjective,
        ECk_ObjectiveStatus NewStatus)
    -> void
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
    HandleObjectiveCompleted(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData)
    -> void
{
    OnObjectiveCompleted(InObjective, InMetaData);
}

auto
    UCk_Objective_EntityScript::
    HandleObjectiveFailed(
        FCk_Handle_Objective InObjective,
        FGameplayTag InMetaData)
    -> void
{
    OnObjectiveFailed(InObjective, InMetaData);
}

// --------------------------------------------------------------------------------------------------------------------