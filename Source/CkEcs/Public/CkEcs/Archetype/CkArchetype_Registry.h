#pragma once

#include "CkEcs/Archetype/CkArchetype_Data.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Process-wide archetype registry — game thread only. Resolution/matching split: CkEcs/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::archetype_registry
{
    using MatcherType = TFunction<bool(const FCk_Handle&)>;

    // Registers (or replaces — last registration wins, logged) an archetype.
    CKECS_API auto Register(const FCk_ArchetypeDescriptor& InDescriptor, MatcherType InNativeMatcher = {}) -> void;

    CKECS_API auto Unregister(FName InName) -> void;

    CKECS_API auto Find(FName InName) -> TOptional<FCk_ArchetypeDescriptor>;

    // Descriptors in precedence order (Priority desc, FeatureIds count desc, registration order).
    CKECS_API auto Get_All() -> TArray<FCk_ArchetypeDescriptor>;

    // FeatureId matching answers only while the DebugFeatureFlags cache is enabled AND every FeatureId
    // is a registered flag; a native matcher, when supplied, takes precedence.
    CKECS_API auto Get_Matches(const FCk_Handle& InHandle, FName InName) -> bool;

    // First matching archetype in precedence order; NAME_None when nothing matches.
    CKECS_API auto TryGet_BestMatchName(const FCk_Handle& InHandle) -> FName;
}
