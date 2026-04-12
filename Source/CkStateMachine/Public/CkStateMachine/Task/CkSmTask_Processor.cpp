#include "CkSmTask_Processor.h"

#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmTask_Tick);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmTask_FireFinishedSignal);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TASK TICK
    // ================================================================================================================

    auto
        FProcessor_SmTask_Tick::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTask_Current& InCurrent)
        -> void
    {
        CK_ENSURE_IF_NOT(InHandle.Has<FFragment_EntityScript_Current>(),
            TEXT("Tick task entity [{}] is missing FFragment_EntityScript_Current — tag should not have been added without a script"), InHandle)
        { return; }

        const auto& ScriptFragment = InHandle.Get<FFragment_EntityScript_Current>();
        auto* Script = ScriptFragment.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Tick task entity [{}] has a null script pointer"), InHandle)
        { return; }

        auto* TaskScript = Cast<UCk_SmTask_EntityScript>(Script);
        CK_ENSURE_IF_NOT(ck::IsValid(TaskScript),
            TEXT("Tick task entity [{}] script is not a UCk_SmTask_EntityScript — wrong script type added with FTag_SmTask_Tick"), InHandle)
        { return; }

        const auto PrevResult = InCurrent._LastResult;
        const auto Result = TaskScript->Tick(InDeltaT.Get_Seconds());
        InCurrent._LastResult = Result;

        if (PrevResult == ECk_SmTaskResult::Running && Result != ECk_SmTaskResult::Running)
        {
            InHandle.AddOrGet<FTag_SmTask_ResultDirty>();
        }
    }

    // ================================================================================================================
    // TASK FIRE FINISHED SIGNAL
    // ================================================================================================================

    auto
        FProcessor_SmTask_FireFinishedSignal::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmTask_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_SmTask_ResultDirty>();

        UUtils_Signal_OnSmTaskFinished::Broadcast(InHandle,
            MakePayload(InHandle, InCurrent.Get_LastResult()));
    }
}

// --------------------------------------------------------------------------------------------------------------------
