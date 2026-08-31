#pragma once

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_Agent);
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_AvoidanceVolume);
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly);
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude);

// The framework's strict planning filter: the permissive default plus the standing-crowd area
// treated as impassable. An agent that names its own strict filter tag outranks this one.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Filter_Crowd_AvoidStandingCrowds);

// --------------------------------------------------------------------------------------------------------------------
