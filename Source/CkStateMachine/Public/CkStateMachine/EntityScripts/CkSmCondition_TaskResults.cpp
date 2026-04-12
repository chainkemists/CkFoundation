#include "CkSmCondition_TaskResults.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_TaskResults::
    BeginPlay()
    -> void
{
    _TickTaskCount = 0;
    _SucceededCount = 0;
    _FailedCount = 0;

    const auto ParentTransition = Get_ParentTransition();
    if (ck::Is_NOT_Valid(ParentTransition))
    {
        Super::BeginPlay();
        return;
    }

    if (NOT ck::TUtils_Sm_ParentState::Has(ParentTransition))
    {
        Super::BeginPlay();
        return;
    }

    const auto ParentState = ck::TUtils_Sm_ParentState::Get_StoredEntity(ParentTransition);
    auto ParentStateHandle = static_cast<FCk_Handle>(ParentState);

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(ParentStateHandle,
        [&](FCk_Handle_SmTask InTask)
        {
            auto TaskHandle = static_cast<FCk_Handle>(InTask);

            if (NOT TaskHandle.Has<ck::FTag_SmTask_Tick>())
            { return; }

            ++_TickTaskCount;

            auto MutableTask = InTask;
            auto Delegate = FCk_Delegate_SmTask_OnFinished{};
            Delegate.BindDynamic(this, &ThisType::OnTaskFinished);
            UCk_Utils_StateMachine_UE::BindTo_OnSmTaskFinished(
                MutableTask,
                Delegate,
                ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
                ECk_Signal_PostFireBehavior::DoNothing);
        });

    Super::BeginPlay();
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
    if (_TickTaskCount == 0)
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
        Satisfied = _SucceededCount == _TickTaskCount;
        break;

    case ECk_SmCondition_TaskResultsCheck::AllFailed:
        Satisfied = _FailedCount == _TickTaskCount;
        break;
    }

    if (Satisfied)
    {
        MarkSatisfied();
    }
}

// --------------------------------------------------------------------------------------------------------------------
