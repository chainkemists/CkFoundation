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
    // Structural fingerprint inputs gathered from a state's DefineState output. Every list is in
    // declaration/call order (ordering is semantic) and _TransitionConditionLists is PARALLEL to
    // _TransitionTargetClasses. _ComposedFromClasses is recorded on top of the flattened output so
    // composing B vs C fingerprints distinctly even when their expansions match.
    struct FFingerprintInputs
    {
        TArray<TSubclassOf<UCk_SmTask_EntityScript>>                _TaskClasses;
        TArray<TSubclassOf<UCk_SmState_EntityScript>>               _TransitionTargetClasses;
        TArray<TArray<TSubclassOf<UCk_SmCondition_EntityScript>>>   _TransitionConditionLists;
        TArray<TSubclassOf<UCk_SmState_EntityScript>>               _ComposedFromClasses;
    };

    // Determinism contract: the same logical DefineState body must produce the same value on every
    // machine, because non-authority machines verify the replicated fingerprint against their locally
    // computed one before applying a transition. int32 (not uint32) because UHT rejects uint32 in
    // BlueprintRead* UPROPERTYs. Hash core and its cross-process constraints: MixClass in the .cpp.
    CKSTATEMACHINE_API auto
    ComputeFingerprint(
        const FFingerprintInputs& InInputs) -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
