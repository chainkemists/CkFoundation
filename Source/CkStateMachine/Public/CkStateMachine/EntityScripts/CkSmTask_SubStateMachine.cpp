#include "CkSmTask_SubStateMachine.h"

#include "CkStateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/CkStateMachine_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_SubStateMachine::
    OnStateEnter()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_InitialStateClass),
        TEXT("SubStateMachine task has no InitialStateClass set"))
    { return; }

    auto TaskEntity = DoGet_ScriptEntity();
    auto GameEntity = DoGet_GameEntity();

    _SubSmHandle = UCk_Utils_StateMachine_UE::Add(
        TaskEntity,
        _InitialStateClass,
        ECk_SmAutoStart::Disabled);

    CK_ENSURE_IF_NOT(ck::IsValid(_SubSmHandle),
        TEXT("Failed to create sub-StateMachine"))
    { return; }

    if (ck::IsValid(GameEntity))
    {
        auto SubSmEntity = static_cast<FCk_Handle>(_SubSmHandle);
        SubSmEntity.Add<ck::FFragment_Sm_Context>(GameEntity);
    }

    auto& SubSmFragment = TaskEntity.AddOrGet<ck::FFragment_SmTask_SubStateMachine>();
    SubSmFragment._SubStateMachineHandle = _SubSmHandle;

    UCk_Utils_StateMachine_UE::Request_Start(_SubSmHandle);

    ck::sm::Verbose(TEXT("[SubStateMachine] Started sub-SM with initial state [{}]"),
        _InitialStateClass->GetName());
}

auto
    UCk_SmTask_SubStateMachine::
    OnStateExit()
    -> void
{
    _SubSmHandle = {};
}

auto
    UCk_SmTask_SubStateMachine::
    Tick(
        float InDeltaSeconds)
    -> ECk_SmTaskResult
{
    if (_CompletionBehavior == ECk_SmTask_SubSm_CompletionBehavior::KeepRunning)
    {
        return ECk_SmTaskResult::Running;
    }

    if (ck::Is_NOT_Valid(_SubSmHandle))
    {
        return ECk_SmTaskResult::Failed;
    }

    auto RunStatus = UCk_Utils_StateMachine_UE::Get_RunStatus(_SubSmHandle);

    if (RunStatus == ECk_SmRunStatus::Stopped)
    {
        return ECk_SmTaskResult::Succeeded;
    }

    return ECk_SmTaskResult::Running;
}

// --------------------------------------------------------------------------------------------------------------------
