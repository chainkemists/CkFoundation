#include "CkSmCondition_TaskResults.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_TaskResults::
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    Super::EnterCondition(InHandle, InNetContext);

    _BoundTaskCount = 0;
    _SucceededCount = 0;
    _FailedCount = 0;
    _BoundTasks.Reset();

    const auto ParentTransition = Get_ParentTransition();
    if (ck::Is_NOT_Valid(ParentTransition))
    { return; }

    if (NOT ck::TUtils_Sm_ParentState::Has(ParentTransition))
    { return; }

    const auto ParentState = ck::TUtils_Sm_ParentState::Get_StoredEntity(ParentTransition);
    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(ParentState,
    [&](FCk_Handle_SmTask InTask)
    {
        ++_BoundTaskCount;

        auto Delegate = FCk_Delegate_SmTask_OnFinished{};
        Delegate.BindDynamic(this, &ThisType::OnTaskFinished);
        UCk_Utils_StateMachine_UE::BindTo_OnSmTaskFinished(
            InTask,
            Delegate,
            ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
            ECk_Signal_PostFireBehavior::DoNothing);
        _BoundTasks.Add(InTask);
    });
}

auto
    UCk_SmCondition_TaskResults::
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    for (auto& BoundTask : _BoundTasks)
    {
        if (NOT ck::IsValid(BoundTask))
        { continue; }

        auto Delegate = FCk_Delegate_SmTask_OnFinished{};
        Delegate.BindDynamic(this, &ThisType::OnTaskFinished);
        UCk_Utils_StateMachine_UE::UnbindFrom_OnSmTaskFinished(BoundTask, Delegate);
    }
    _BoundTasks.Reset();

    Super::ExitCondition(InHandle, InNetContext);
}

void
    UCk_SmCondition_TaskResults::
    OnTaskFinished(
        FCk_Handle_SmTask InTaskHandle,
        ECk_SmTaskResult InResult)
{
    if (InResult == ECk_SmTaskResult::Succeeded)
    {
        ++_SucceededCount;
    }
    else if (InResult == ECk_SmTaskResult::Failed)
    {
        ++_FailedCount;
    }

    DoEvaluateThreshold();
}

void
    UCk_SmCondition_TaskResults::
    DoEvaluateThreshold()
{
    if (_BoundTaskCount == 0)
    { return; }

    auto Satisfied = false;

    switch (_Check)
    {
    case ECk_SmCondition_TaskResultsCheck::AnySucceeded:
        Satisfied = _SucceededCount > 0;
        break;

    case ECk_SmCondition_TaskResultsCheck::AnyFailed:
        Satisfied = _FailedCount > 0;
        break;

    case ECk_SmCondition_TaskResultsCheck::AllSucceeded:
        Satisfied = _SucceededCount == _BoundTaskCount;
        break;

    case ECk_SmCondition_TaskResultsCheck::AllFailed:
        Satisfied = _FailedCount == _BoundTaskCount;
        break;
    }

    if (Satisfied)
    {
        MarkSatisfied();
    }
}

// --------------------------------------------------------------------------------------------------------------------
