#pragma once

#include "CkNav_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_constants
{
    constexpr int32 Fallback_MaxCrowdAgents             = 64;
    constexpr float Fallback_MaxAgentRadius             = 200.0f;
    constexpr float Fallback_NavQuerySearchHalfExtent   = 250.0f;
    constexpr int32 Fallback_MaxPathQueriesPerFrame     = 8;
    constexpr float Fallback_TeleportThresholdUu        = 10.0f;
    constexpr float Fallback_NavRebuildDebounceSeconds  = 0.5f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_ProjectSettings::
    Get_MaxCrowdAgents()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_nav_constants::Fallback_MaxCrowdAgents; }

    return Settings->Get_MaxCrowdAgents();
}

auto
    UCk_Utils_Nav_ProjectSettings::
    Get_MaxAgentRadius()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_nav_constants::Fallback_MaxAgentRadius; }

    return Settings->Get_MaxAgentRadius();
}

auto
    UCk_Utils_Nav_ProjectSettings::
    Get_NavQuerySearchHalfExtent()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_nav_constants::Fallback_NavQuerySearchHalfExtent; }

    return Settings->Get_NavQuerySearchHalfExtent();
}

auto
    UCk_Utils_Nav_ProjectSettings::
    Get_MaxPathQueriesPerFrame()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_nav_constants::Fallback_MaxPathQueriesPerFrame; }

    return Settings->Get_MaxPathQueriesPerFrame();
}

auto
    UCk_Utils_Nav_ProjectSettings::
    Get_TeleportThresholdUu()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_nav_constants::Fallback_TeleportThresholdUu; }

    return Settings->Get_TeleportThresholdUu();
}

auto
    UCk_Utils_Nav_ProjectSettings::
    Get_NavRebuildDebounceSeconds()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_nav_constants::Fallback_NavRebuildDebounceSeconds; }

    return Settings->Get_NavRebuildDebounceSeconds();
}

// --------------------------------------------------------------------------------------------------------------------
