#pragma once

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CKNAVIGATION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Restricted);

// The well-known impassable area. Every provider maps it to whatever removes walkable surface
// outright, so an entity painting it blocks EVERY agent regardless of query filter - unlike
// TAG_Nav_Area_Restricted, whose traversal semantics each filter decides for itself.
CKNAVIGATION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Impassable);

// The well-known named query filter for entities that plan like a customer would. The framework
// registers no definition for it - the consuming project decides what the name means - but the
// tag itself is declared here so it resolves by name on every machine.
CKNAVIGATION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Filter_Customer);

// --------------------------------------------------------------------------------------------------------------------
