#pragma once

#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment_Data.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkStateMachine_Debug_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Debug-side mirror of UCk_Utils_StateMachine_UE — exposes Request_* entry points that
// push FCk_Request_SmDebug_* into FFragment_SmDebug_Requests. The core state machine
// code calls these instead of reaching into the debug processor directly.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKSTATEMACHINE_API UCk_Utils_StateMachineDebug_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Queue a "record this transition in history" request on the SM entity. Called the
    // moment a transition fires — supports same-frame pumps (A→B→C) where each record
    // is drained in order by the debug handle-requests processor.
    static auto
    Request_RecordTransition(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Request_SmDebug_RecordTransition& InRequest) -> FCk_Handle_StateMachine;

    // Demand tracking for the Sm_Debug poll processor. Pull-based debugger consumers (SM
    // Debugger window, ECS Debugger inspector) stamp each read of FFragment_Sm_Debug data
    // here; the poll processor skips its whole view iteration when nothing consumed
    // recently AND the on-screen debug overlay (ck.DebugOverlay) is off. Game thread only.
    static auto
    NotifyDebugDataConsumed() -> void;

    static auto
    Get_IsDebugDataDesired() -> bool;

private:
    static auto
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest) -> FCk_Handle_StateMachine;
};

// --------------------------------------------------------------------------------------------------------------------
