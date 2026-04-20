#include "CkSmTask_DebugLog.h"

#include "CkStateMachine/CkStateMachine_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_DebugLog::
    EnterTask(
        FCk_Handle_SmTask InHandle)
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Enter: {}"), _EnterMessage);
    Super::EnterTask(InHandle);
}

auto
    UCk_SmTask_DebugLog::
    ExitTask(
        FCk_Handle_SmTask InHandle)
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Exit: {}"), _ExitMessage);
    Super::ExitTask(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
