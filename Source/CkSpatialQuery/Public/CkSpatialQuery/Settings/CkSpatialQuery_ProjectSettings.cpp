#pragma once

#include "CkSpatialQuery_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

// Fallback values used when the CDO is unavailable (e.g. during early engine init).
// These are intentionally conservative — e.g. parallel physics falls back to disabled.
namespace ck_spatial_query_constants
{
    constexpr int32 Fallback_MaxBodies = 65536;
    constexpr int32 Fallback_MaxBodyPairs = 65536;
    constexpr int32 Fallback_MaxContactConstraints = 10240;
    constexpr int32 Fallback_TempAllocatorSizeMB = 10;
    constexpr int32 Fallback_MaxPhysicsJobs = 2048;
    constexpr int32 Fallback_MaxPhysicsBarriers = 8;
    constexpr int32 Fallback_CollisionSteps = 1;
    constexpr bool  Fallback_EnableParallelPhysics = false;
    constexpr int32 Fallback_NumPhysicsThreads = 0;
    constexpr bool  Fallback_EnableAsyncPhysicsUpdate = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxBodies()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_MaxBodies; }

    return Settings->Get_MaxBodies();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxBodyPairs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_MaxBodyPairs; }

    return Settings->Get_MaxBodyPairs();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxContactConstraints()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_MaxContactConstraints; }

    return Settings->Get_MaxContactConstraints();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_TempAllocatorSizeMB()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_TempAllocatorSizeMB; }

    return Settings->Get_TempAllocatorSizeMB();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxPhysicsJobs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_MaxPhysicsJobs; }

    return Settings->Get_MaxPhysicsJobs();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_MaxPhysicsBarriers()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_MaxPhysicsBarriers; }

    return Settings->Get_MaxPhysicsBarriers();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_CollisionSteps()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_CollisionSteps; }

    return Settings->Get_CollisionSteps();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_EnableParallelPhysics()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_EnableParallelPhysics; }

    return Settings->Get_EnableParallelPhysics();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_NumPhysicsThreads()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_NumPhysicsThreads; }

    return Settings->Get_NumPhysicsThreads();
}

auto
    UCk_Utils_SpatialQuery_ProjectSettings::
    Get_EnableAsyncPhysicsUpdate()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_SpatialQuery_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_spatial_query_constants::Fallback_EnableAsyncPhysicsUpdate; }

    return Settings->Get_EnableAsyncPhysicsUpdate();
}

// --------------------------------------------------------------------------------------------------------------------
