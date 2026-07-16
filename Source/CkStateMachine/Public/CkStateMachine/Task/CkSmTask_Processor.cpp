#include "CkSmTask_Processor.h"

#include "CkStateMachine/CkStateMachine_Stats.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_SmTask_Tick);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmTask_FireFinishedSignal);
CK_REGISTER_PROCESSOR(ck::FProcessor_SmTask_Exit);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("SmTask::Tick (proc)"), STAT_SmTask_TickProc, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmTask::Exit"), STAT_SmTask_Exit, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmTask::FireFinishedSignal"), STAT_SmTask_FireFinishedSignal, STATGROUP_CkStateMachine);
DECLARE_CYCLE_STAT(TEXT("SmTask::Tick (user)"), STAT_SmTask_Tick, STATGROUP_CkStateMachine);
DECLARE_DWORD_COUNTER_STAT(TEXT("Sm Tasks Ticked"), STAT_SmTasksTicked, STATGROUP_CkStateMachine);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SmTask_Tick::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_SmTask_Current& InCurrent,
            const FFragment_EntityScript_Current& InScriptFragment)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmTask_TickProc);

        auto* Script = InScriptFragment.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Tick task entity [{}] has a null script pointer"), InHandle)
        { return; }

        auto* TaskScript = Cast<UCk_SmTask_EntityScript>(Script);
        CK_ENSURE_IF_NOT(ck::IsValid(TaskScript),
            TEXT("Tick task entity [{}] script is not a UCk_SmTask_EntityScript — wrong script type added with FTag_SmTask_Tick"), InHandle)
        { return; }

        const auto SmHandle = UCk_Utils_SmTask_UE::Get_OwningStateMachine(InHandle);

        // Invariant: a task never outlives its owning SM (destroy cascades mark owner + dependents
        // together). A destroyed owner under a live, ticking task means someone created or kept this
        // task outside its owner's lifetime cascade — name both so the creator can be found, instead
        // of the former symptom (a tombstone-ensure storm from inside ComputeNetContext).
        CK_ENSURE_IF_NOT(ck::IsValid(SmHandle),
            TEXT("SmTask [{}] (script [{}]) is alive and ticking but its owning StateMachine [{}] is destroyed — "
                 "a task must never outlive its owning SM (broken lifetime cascade)"),
            InHandle, TaskScript->GetClass(), SmHandle)
        { return; }

        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);

        // Authority gating (spec §5/§6): only the machine that owns this SM's transitions ticks
        // tasks. Non-authority machines replay state-entry/exit via the replicated history; their
        // tasks must not produce side effects locally. Uses the shared transition-authority
        // predicate — the prior inline gate only skipped NonOwningClient and non-OwningClientAuth
        // OwningClient, so the server of an OwningClientAuth SM (which must follow the relay,
        // listen-host-owns-pawn excepted) still ticked tasks.
        if (NOT ck::statemachine::Get_IsTransitionAuthority(SmHandle))
        { return; }

        INC_DWORD_STAT(STAT_SmTasksTicked);

        const auto Result = [&]
        {
            SCOPE_CYCLE_COUNTER(STAT_SmTask_Tick);
            return TaskScript->Tick(InHandle, InDeltaT, NetContext);
        }(); // immediately-invoked so the scope times only the user Tick, not Request_UpdateTaskResult

        UCk_Utils_SmTask_UE::Request_UpdateTaskResult(InHandle, Result);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SmTask_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InScriptFragment)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmTask_Exit);

        auto* Script = Cast<UCk_SmTask_EntityScript>(InScriptFragment.Get_Script().Get());
        if (ck::Is_NOT_Valid(Script))
        {
            // Still clear the dirty key — this processor is MarkedDirtyBy the tag, so leaving it
            // makes the entity re-match every pump until its deferred destroy lands.
            InHandle.Try_Remove<FTag_SmTask_PendingExit>();
            return;
        }

        const auto SmHandle = UCk_Utils_SmTask_UE::Get_OwningStateMachine(InHandle);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
        Script->ExitTask(InHandle, NetContext);
        InHandle.Try_Remove<FTag_SmTask_PendingExit>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_SmTask_FireFinishedSignal::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmTask_Current& InCurrent)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_SmTask_FireFinishedSignal);

        InHandle.Remove<FTag_SmTask_ResultDirty>();

        UUtils_Signal_OnSmTaskFinished::Broadcast(InHandle,
            MakePayload(InHandle, InCurrent.Get_LastResult()));
    }
}

// --------------------------------------------------------------------------------------------------------------------
