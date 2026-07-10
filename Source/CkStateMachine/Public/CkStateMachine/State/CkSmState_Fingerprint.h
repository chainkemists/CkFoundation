#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Templates/SubclassOf.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmTask_EntityScript;
class UCk_SmCondition_EntityScript;
class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::statemachine
{
    // Structural fingerprint inputs gathered from a state's DefineState output.
    //
    // - _TaskClasses: declaration-order. Task ordering is semantic (Enter/Tick/Exit ordering
    //   is observable to gameplay code).
    // - _TransitionTargetClasses: declaration-order, one entry per AddTransition call.
    // - _TransitionConditionLists: parallel to _TransitionTargetClasses; each inner array is
    //   the transition's condition classes in declaration order.
    // - _ComposedFromClasses: classes passed to ComposeFromState() during DefineState, captured
    //   in call order. The recursive expansion of those CDOs' DefineState bodies populates
    //   tasks/transitions/conditions on the host state — those land in the lists above. The
    //   compose list itself is recorded so a state that composes B vs C produces a distinct
    //   fingerprint even when their flattened outputs happen to match.
    struct FFingerprintInputs
    {
        TArray<TSubclassOf<UCk_SmTask_EntityScript>>                _TaskClasses;
        TArray<TSubclassOf<UCk_SmState_EntityScript>>               _TransitionTargetClasses;
        TArray<TArray<TSubclassOf<UCk_SmCondition_EntityScript>>>   _TransitionConditionLists;
        TArray<TSubclassOf<UCk_SmState_EntityScript>>               _ComposedFromClasses;
    };

    // Computes a deterministic 32-bit fingerprint from structural inputs.
    //
    // Determinism contract (spec §9): the same _logical_ DefineState body executed on any
    // machine — server, owning client, non-owning client — must produce the same int32 value
    // because non-authority machines verify the replicated fingerprint against their locally
    // computed one before applying a transition.
    //
    // Hash core: FNV-1a mix over each class's case-normalized NAME STRING. The string is the
    // only class identity that is stable across processes — FName comparison indices are
    // process-local (assigned in name-table population order) and an FName's stored case is
    // first-registration-wins, so neither survives a packaged-client vs dedicated-server
    // comparison (see MixClass in the .cpp for the full rationale).
    //
    // Returned as int32 to match FFragment_SmState_Fingerprint._Hash and
    // FCk_Sm_TransitionEvent._NewStateFingerprint, both of which are int32 because UHT
    // rejects uint32 in BlueprintReadOnly/BlueprintReadWrite UPROPERTYs.
    CKSTATEMACHINE_API auto
    ComputeFingerprint(
        const FFingerprintInputs& InInputs) -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
