#pragma once

// --------------------------------------------------------------------------------------------------------------------
// PRIVATE despite living under Public/ — include only from CkGoap_Planner_Utils.cpp.

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

namespace ck::goap::internal_planner
{
    // Returns the EXISTING Action handle (with a warning) when the class is already in the
    // Planner's catalog. Wires no parent/child tree edges and does not seed the active chain.
    CKGOAP_API auto
    DoCreateOrFindActionEntity(
        FCk_Handle_Goap_Planner& InPlanner,
        const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action;

    // Resolution order: the child's own override → the parent's resolved → the owning Planner's
    // default. Eager, so the child's Setup can run before any parent plan is requested.
    CKGOAP_API auto
    DoResolveChildWorldStateFromParent(
        FCk_Handle_Goap_Action& InChild,
        const FCk_Handle_Goap_Action& InParentAction) -> void;
}
