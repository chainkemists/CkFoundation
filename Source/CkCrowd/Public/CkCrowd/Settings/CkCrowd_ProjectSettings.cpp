#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Crowd_Settings_UE::Get() -> const UCk_Crowd_ProjectSettings_UE*
{
    return GetDefault<UCk_Crowd_ProjectSettings_UE>();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AccelClampMode() -> ECk_AccelClampMode
{
    return Get()->Get_AccelClampMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleTrigger() -> ECk_AvoidanceSampleTrigger
{
    return Get()->Get_AvoidanceSampleTrigger();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleNeighborThreshold() -> int32
{
    return Get()->Get_AvoidanceSampleNeighborThreshold();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleStride() -> int32
{
    return Get()->Get_AvoidanceSampleStride();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleDepth() -> int32
{
    return Get()->Get_AvoidanceSampleDepth();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceWallSegments() -> ECk_AvoidanceWallSegmentsMode
{
    return Get()->Get_AvoidanceWallSegments();
}

auto UCk_Utils_Crowd_Settings_UE::Get_PushApartMode() -> ECk_PushApartMode
{
    return Get()->Get_PushApartMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_NavmeshConstraintMode() -> ECk_CrowdNavmeshConstraintMode
{
    return Get()->Get_NavmeshConstraintMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_StationaryMarkupMode() -> ECk_CrowdStationaryMarkupMode
{
    return Get()->Get_StationaryMarkupMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_PathRefreshMode() -> ECk_CrowdPathRefreshMode
{
    return Get()->Get_PathRefreshMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_BlockDetectionMode() -> ECk_CrowdBlockDetectionMode
{
    return Get()->Get_BlockDetectionMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_CrowdedGoalBlockMode() -> ECk_CrowdCrowdedGoalBlockMode
{
    return Get()->Get_CrowdedGoalBlockMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_WaypointRetirementLineOfSight() -> ECk_CrowdWaypointRetirementLineOfSightMode
{
    return Get()->Get_WaypointRetirementLineOfSight();
}

// --------------------------------------------------------------------------------------------------------------------
