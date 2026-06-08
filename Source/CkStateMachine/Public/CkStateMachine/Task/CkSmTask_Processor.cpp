#include "CkSmTask_Processor.h"

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
            FFragment_SmTask_Current& InCurrent,
            const FFragment_EntityScript_Current& InScriptFragment)
        -> void
    {
        auto* Script = InScriptFragment.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Tick task entity [{}] has a null script pointer"), InHandle)
        { return; }

        auto* TaskScript = Cast<UCk_SmTask_EntityScript>(Script);
        CK_ENSURE_IF_NOT(ck::IsValid(TaskScript),
            TEXT("Tick task entity [{}] script is not a UCk_SmTask_EntityScript — wrong script type added with FTag_SmTask_Tick"), InHandle)
        { return; }

        const auto SmHandle = UCk_Utils_SmTask_UE::Get_OwningStateMachine(InHandle);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);

        // Authority gating (spec §5/§6): only the machine that owns this SM's transitions ticks
        // tasks. Non-owning clients replay state-entry/exit via the replicated history; their
        // tasks must not produce side effects locally. Standalone is always authority. Server is
        // authority for ServerAuth SMs. OwningClient is authority only for OwningClientAuth SMs.
        // NonOwningClient is never authority.
        if (NetContext == ECk_Sm_NetContext::NonOwningClient)
        { return; }

        if (NetContext == ECk_Sm_NetContext::OwningClient
            && UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(SmHandle)
                != ECk_Sm_AuthorityModel::OwningClientAuthoritative)
        { return; }

        const auto Result = TaskScript->Tick(InHandle, InDeltaT, NetContext);

        UCk_Utils_SmTask_UE::Request_UpdateTaskResult(InHandle, Result);
    }

    // ================================================================================================================
    // TASK EXIT
    // ================================================================================================================

    auto
        FProcessor_SmTask_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InScriptFragment)
        -> void
    {
        auto* Script = Cast<UCk_SmTask_EntityScript>(InScriptFragment.Get_Script().Get());
        if (ck::Is_NOT_Valid(Script))
        { return; }

        const auto SmHandle = UCk_Utils_SmTask_UE::Get_OwningStateMachine(InHandle);
        const auto NetContext = ck::statemachine::ComputeNetContext(SmHandle);
        Script->ExitTask(InHandle, NetContext);
        InHandle.Try_Remove<FTag_SmTask_PendingExit>();
    }

    // ================================================================================================================
    // TASK FIRE FINISHED SIGNAL
    // ================================================================================================================

    auto
        FProcessor_SmTask_FireFinishedSignal::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_SmTask_Current& InCurrent)
        -> void
    {
        InHandle.Remove<FTag_SmTask_ResultDirty>();

        UUtils_Signal_OnSmTaskFinished::Broadcast(InHandle,
            MakePayload(InHandle, InCurrent.Get_LastResult()));
    }
}

// --------------------------------------------------------------------------------------------------------------------
