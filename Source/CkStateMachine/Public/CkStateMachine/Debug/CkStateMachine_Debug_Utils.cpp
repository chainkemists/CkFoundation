#include "CkStateMachine_Debug_Utils.h"

#include "CkStateMachine/Debug/CkStateMachine_Debug_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    Request_RecordTransition(
        FCk_Handle_StateMachine& InStateMachine,
        const FCk_Request_SmDebug_RecordTransition& InRequest)
    -> FCk_Handle_StateMachine
{
    return DoAddRequest(InStateMachine, InRequest);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateMachineDebug_UE::
    DoAddRequest(
        FCk_Handle_StateMachine& InStateMachine,
        const auto& InRequest)
    -> FCk_Handle_StateMachine
{
    auto& Requests = InStateMachine.AddOrGet<ck::FFragment_SmDebug_Requests>();
    Requests._Requests.Add(InRequest);
    return InStateMachine;
}
