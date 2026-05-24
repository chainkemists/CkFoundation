#include "CkStateMachineRelay_Actor.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_StateMachineRelay_UE::
    Server_PushTransitionBatch_Implementation(
        FCk_Handle InSMHandle,
        const TArray<FCk_Sm_TransitionEvent>& InBatch)
    -> void
{
    ck::sm::Verbose(TEXT("Relay received PushTransitionBatch for SM [{}] with [{}] entries"),
        InSMHandle, InBatch.Num());

    // Wired in Phase 10 (owning-client authoritative path). Stub for now.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_StateMachineRelay_UE::
    Server_PushCurrentState_Implementation(
        FCk_Handle InSMHandle,
        TSubclassOf<UCk_SmState_EntityScript> InCurrentStateClass,
        int32 InSeq,
        int32 InCurrentStateFingerprint)
    -> void
{
    ck::sm::Verbose(TEXT("Relay received PushCurrentState for SM [{}] state [{}] seq [{}] fp [{}]"),
        InSMHandle, InCurrentStateClass, InSeq, InCurrentStateFingerprint);

    // Wired in Phase 10. Stub for now.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_StateMachineRelay_UE::
    Server_PushRunStatus_Implementation(
        FCk_Handle InSMHandle,
        ECk_SmRunStatus InRunStatus)
    -> void
{
    ck::sm::Verbose(TEXT("Relay received PushRunStatus for SM [{}] status [{}]"),
        InSMHandle, InRunStatus);

    // Wired in Phase 10 / Phase 11. Stub for now.
}

// --------------------------------------------------------------------------------------------------------------------
