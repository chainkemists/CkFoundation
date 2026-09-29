#pragma once

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_Agent);
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_AvoidanceVolume);
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_AvoidanceVolume_CostOnly);
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Area_Crowd_AvoidanceVolume_HardExclude);

// Excludes only the standing-crowd area. The planner no longer uses it; kept for tests and hosts.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Nav_Filter_Crowd_AvoidStandingCrowds);

// --------------------------------------------------------------------------------------------------------------------
