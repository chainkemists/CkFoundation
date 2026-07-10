#pragma once

#include "CkEcs/Archetype/CkArchetype_Data.h"
#include "CkEcs/Handle/CkHandle.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Process-wide archetype registry (game thread only). See CkArchetype_Data.h for the
// resolution split.
//
// CkEcs-side matching covers: the native matcher (when supplied), else FeatureIds via the
// DebugFeatureFlags bit cache — which means Get_Matches only answers while the cache is
// enabled AND every FeatureId is a registered flag. RequiredLabel/NamePattern refinement
// is consumer-side. Bulk matching over many entities should resolve the required mask
// once via debug_feature_flags::Get_BitIndex and test rows directly; Get_Matches is the
// convenience path (BP/AS, single queries).
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

    CKECS_API auto Get_Matches(const FCk_Handle& InHandle, FName InName) -> bool;

    // First matching archetype in precedence order; NAME_None when nothing matches.
    CKECS_API auto TryGet_BestMatchName(const FCk_Handle& InHandle) -> FName;
}
