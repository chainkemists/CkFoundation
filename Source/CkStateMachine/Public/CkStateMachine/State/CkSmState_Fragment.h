#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ================================================================================================================
    // TAGS
    // ================================================================================================================

    // State evaluation model — every state defaults to EventDriven on entry.
    // Mark a state as Ticking to opt in to per-pump polled evaluation.
    CK_DEFINE_ECS_TAG(FTag_SmState_EventDriven);
    CK_DEFINE_ECS_TAG(FTag_SmState_Ticking);

    // Marks the currently active state of a running SM.
    // Added by UCk_Utils_SmState_UE::Create, removed when the state entity is destroyed on exit.
    CK_DEFINE_ECS_TAG(FTag_SmState_Active);

    // ================================================================================================================
    // RECORDS
    // ================================================================================================================

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSmStates, FCk_Handle_SmState);

    // ================================================================================================================
    // ENTITY-HOLDER BACK-REFERENCES
    // ================================================================================================================

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Sm_ParentState, FFragment_Sm_ParentState, FCk_Handle_SmState);
}

// --------------------------------------------------------------------------------------------------------------------
