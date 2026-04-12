#include "CkSmState_Utils.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/Transition/CkSmTransition_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle)
        && (InHandle.Has<ck::FTag_SmState_EventDriven>() || InHandle.Has<ck::FTag_SmState_Ticking>());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    MarkStateAs_Ticking(
        FCk_Handle_SmState& InState)
    -> FCk_Handle_SmState
{
    CK_ENSURE_IF_NOT(ck::IsValid(InState),
        TEXT("Invalid state handle in MarkStateAs_Ticking"))
    { return InState; }

    auto StateHandle = static_cast<FCk_Handle>(InState);
    StateHandle.Try_Remove<ck::FTag_SmState_EventDriven>();
    StateHandle.AddOrGet<ck::FTag_SmState_Ticking>();

    return InState;
}

auto
    UCk_Utils_SmState_UE::
    MarkStateAs_EventDriven(
        FCk_Handle_SmState& InState)
    -> FCk_Handle_SmState
{
    CK_ENSURE_IF_NOT(ck::IsValid(InState),
        TEXT("Invalid state handle in MarkStateAs_EventDriven"))
    { return InState; }

    auto StateHandle = static_cast<FCk_Handle>(InState);
    StateHandle.Try_Remove<ck::FTag_SmState_Ticking>();
    StateHandle.AddOrGet<ck::FTag_SmState_EventDriven>();

    return InState;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SmState_UE::
    Get_IsReadyToTransition(
        const FCk_Handle_SmState& InState)
    -> bool
{
    if (ck::Is_NOT_Valid(InState))
    { return false; }

    auto StateHandle = static_cast<FCk_Handle>(InState);
    auto IsReady = false;

    // A state is ready to transition when any of its transitions has Pass result
    for (const auto& ChildHandle : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(StateHandle))
    {
        if (NOT ChildHandle.Has<ck::FFragment_SmTransition_Current>())
        { continue; }

        if (ChildHandle.Get<ck::FFragment_SmTransition_Current>().Get_Result() == ECk_SmTransitionResult::Pass)
        {
            IsReady = true;
            break;
        }
    }

    return IsReady;
}

auto
    UCk_Utils_SmState_UE::
    Get_OwningStateMachine(
        const FCk_Handle_SmState& InState)
    -> FCk_Handle_StateMachine
{
    if (ck::Is_NOT_Valid(InState))
    { return {}; }

    auto StateHandle = static_cast<FCk_Handle>(InState);

    if (ck::TUtils_Sm_OwningStateMachine::Has(StateHandle))
    { return ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(StateHandle); }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
