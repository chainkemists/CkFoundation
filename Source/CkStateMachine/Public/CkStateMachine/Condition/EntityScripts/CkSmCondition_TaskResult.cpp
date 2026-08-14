#include "CkSmCondition_TaskResult.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_TaskResult::
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    Super::EnterCondition(InHandle, InNetContext);

    if (ck::Is_NOT_Valid(_TaskClass))
    { return; }

    const auto ParentTransition = Get_ParentTransition();
    if (ck::Is_NOT_Valid(ParentTransition))
    { return; }

    if (NOT ck::TUtils_Sm_ParentState::Has(ParentTransition))
    { return; }

    const auto ParentState = ck::TUtils_Sm_ParentState::Get_StoredEntity(ParentTransition);
    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(ParentState,
    [&](FCk_Handle_SmTask InTask)
    {
        if (NOT InTask.Has<ck::FFragment_EntityScript_Current>())
        { return; }

        auto* Script = InTask.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
        if (ck::Is_NOT_Valid(Script))
        { return; }

        if (Script->GetClass() != _TaskClass.Get())
        { return; }

        auto MutableTask = InTask;
        auto Delegate = FCk_Delegate_SmTask_OnFinished{};
        Delegate.BindDynamic(this, &ThisType::OnTaskFinished);
        UCk_Utils_StateMachine_UE::BindTo_OnSmTaskFinished(
            MutableTask,
            Delegate,
            ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
            ECk_Signal_PostFireBehavior::DoNothing);
        _BoundTasks.Add(InTask);
    });
}

auto
    UCk_SmCondition_TaskResult::
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    for (auto& BoundTask : _BoundTasks)
    {
        if (ck::Is_NOT_Valid(BoundTask))
        { continue; }

        auto Delegate = FCk_Delegate_SmTask_OnFinished{};
        Delegate.BindDynamic(this, &ThisType::OnTaskFinished);
        UCk_Utils_StateMachine_UE::UnbindFrom_OnSmTaskFinished(BoundTask, Delegate);
    }
    _BoundTasks.Reset();

    Super::ExitCondition(InHandle, InNetContext);
}

void
    UCk_SmCondition_TaskResult::
    OnTaskFinished(
        FCk_Handle_SmTask InTaskHandle,
        ECk_SmTaskResult InResult)
{
    if (InResult == _ExpectedResult)
    {
        MarkSatisfied();
    }
    else
    {
        MarkUnsatisfied();
    }
}

// --------------------------------------------------------------------------------------------------------------------
