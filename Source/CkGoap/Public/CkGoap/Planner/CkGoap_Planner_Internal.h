#pragma once

// ====================================================================================================================
// PRIVATE: shared internal helper for Planner/Action entity creation.
//
// Single entity-creation primitive used by AddAction. Stamps the per-entity
// fragment cluster (Params, Current, Definition, Tree, Requests, plus the
// Planner-role-on-Action cluster for the dual-role default — PlanState, Goal,
// WorldStateSource, Activation) and registers the new entity in the owning
// Planner's catalog + record. Does NOT wire any parent/child tree edges or
// flip activation state — that's AddAction's job.
//
// Included only by:
//   - CkGoap_Planner_Utils.cpp
//
// Not a public header.
// ====================================================================================================================

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"     // FCk_Handle_Goap_Action, FCk_Fragment_Goap_ActionParamsData
#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"  // FCk_Handle_Goap_Planner
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"

namespace ck::goap::internal_planner
{
    // Creates an Action entity in the given Planner's catalog. Mirrors the
    // entity-construction half of AddAction — fragments, tag-label,
    // record entry, catalog-index entry, RequiresSetup tag. Does NOT seed the
    // active chain or wire any parent/child tree edges (callers do that).
    //
    // Returns the existing Action handle if the class is already registered
    // in the Planner's catalog (warn-and-return-existing semantics).
    CKGOAP_API auto
    DoCreateOrFindActionEntity(
        FCk_Handle_Goap_Planner& InPlanner,
        const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action;

    // PR-A: resolve a newly-created child Action's WorldStateSource by walking
    // override → parent's resolved → owning Planner's default. Used by
    // AddAction to eagerly resolve children's WS so their Setup processors can
    // run before any parent plan is requested. Defined in CkGoap_Planner_Utils.cpp.
    CKGOAP_API auto
    DoResolveChildWorldStateFromParent(
        FCk_Handle_Goap_Action& InChild,
        const FCk_Handle_Goap_Action& InParentAction) -> void;
}
