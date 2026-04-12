#include "CkSmCondition_TaskResult.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_TaskResult::
    BeginPlay()
    -> void
{
    if (ck::Is_NOT_Valid(_TaskClass))
    {
        Super::BeginPlay();
        return;
    }

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

            if (NOT TaskHandle.Has<ck::FFragment_EntityScript_Current>())
            { return; }

            auto* Script = TaskHandle.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
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
        });

    Super::BeginPlay();
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
