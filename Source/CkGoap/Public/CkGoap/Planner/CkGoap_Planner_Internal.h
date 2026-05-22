#pragma once

// ====================================================================================================================
// PRIVATE: shared internal helpers for ActionSet/Action entity creation.
//
// Phase U2: the ActionSet/Action unification refactor needed a single
// entity-creation primitive shared between AddAction (creates an
// Action under the ActionSet) and AddAction_ToAction (creates a child Action
// under a parent Action — but still resides in the ActionSet's catalog).
//
// Included only by:
//   - CkGoap_Planner_Utils.cpp
//   - CkGoap_Action_Utils.cpp
//
// Not a public header — keeps the CkRecord dependency private.
// ====================================================================================================================

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"     // FCk_Handle_Goap_Action, FCk_Fragment_Goap_ActionParamsData
#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"  // FCk_Handle_Goap_Planner
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

namespace ck::goap::internal_planner
{
    // Creates an Action entity in the given ActionSet's catalog. Mirrors the
    // entity-construction half of AddAction — fragments, tag-label,
    // record entry, catalog-index entry, RequiresSetup tag. Does NOT seed the
    // active chain or wire any parent/child tree edges (callers do that).
    //
    // Returns the existing Action handle if the class is already registered
    // in the ActionSet's catalog (warn-and-return-existing semantics).
    CKGOAP_API auto
    DoCreateOrFindActionEntity(
        FCk_Handle_Goap_Planner& InPlanner,
        const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action;
}
