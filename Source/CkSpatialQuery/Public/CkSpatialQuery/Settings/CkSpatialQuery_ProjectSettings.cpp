#pragma once

#include "CkSpatialQuery_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxBodies()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 65536; }

    return Settings->Get_MaxBodies();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxBodyPairs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 65536; }

    return Settings->Get_MaxBodyPairs();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxContactConstraints()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 10240; }

    return Settings->Get_MaxContactConstraints();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_TempAllocatorSizeMB()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 10; }

    return Settings->Get_TempAllocatorSizeMB();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxPhysicsJobs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 2048; }

    return Settings->Get_MaxPhysicsJobs();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxPhysicsBarriers()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 8; }

    return Settings->Get_MaxPhysicsBarriers();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_CollisionSteps()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 1; }

    return Settings->Get_CollisionSteps();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_bEnableParallelPhysics()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_bEnableParallelPhysics();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_NumPhysicsThreads()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 0; }

    return Settings->Get_NumPhysicsThreads();
}

// --------------------------------------------------------------------------------------------------------------------
