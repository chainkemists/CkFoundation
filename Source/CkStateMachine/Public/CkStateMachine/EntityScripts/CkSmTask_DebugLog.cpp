#include "CkSmTask_DebugLog.h"

#include "CkStateMachine/CkStateMachine_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_DebugLog::
    BeginPlay()
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Enter: {}"), _EnterMessage);
    Super::BeginPlay();
}

auto
    UCk_SmTask_DebugLog::
    EndPlay()
    -> void
{
    ck::sm::Verbose(TEXT("[DebugLog] Exit: {}"), _ExitMessage);
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------
