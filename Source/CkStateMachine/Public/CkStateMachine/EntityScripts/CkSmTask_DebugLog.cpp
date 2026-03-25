#include "CkSmTask_DebugLog.h"

#include "CkStateMachine/CkStateMachine_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_DebugLog::
    OnStateEnter()
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Enter: {}"), _EnterMessage);
}

auto
    UCk_SmTask_DebugLog::
    OnStateExit()
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Exit: {}"), _ExitMessage);
}

// --------------------------------------------------------------------------------------------------------------------
