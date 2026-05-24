#include "CkSmTask_DebugLog.h"

#include "CkStateMachine/CkStateMachine_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_DebugLog::
    EnterTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Enter: {}"), _EnterMessage);
    Super::EnterTask(InHandle, InNetContext);
}

auto
    UCk_SmTask_DebugLog::
    ExitTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Exit: {}"), _ExitMessage);
    Super::ExitTask(InHandle, InNetContext);
}

// --------------------------------------------------------------------------------------------------------------------
