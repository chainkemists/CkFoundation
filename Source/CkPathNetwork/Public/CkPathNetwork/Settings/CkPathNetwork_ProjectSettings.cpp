#include "CkPathNetwork_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_Settings_UE::
    Get_MaxRouteQueriesPerFrame()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PathNetwork_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 4; }
    return Settings->Get_MaxRouteQueriesPerFrame();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_Settings_UE::
    Get_GoalCandidateCount()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PathNetwork_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 5; }
    return Settings->Get_GoalCandidateCount();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_Settings_UE::
    Get_CandidateSearchRadius()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PathNetwork_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 3000.0f; }
    return Settings->Get_CandidateSearchRadius();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_Settings_UE::
    Get_CandidateSearchMaxDoublings()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PathNetwork_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 3; }
    return Settings->Get_CandidateSearchMaxDoublings();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_Settings_UE::
    Get_RepriceTolerance()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PathNetwork_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 1.5f; }
    return Settings->Get_RepriceTolerance();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetwork_Settings_UE::
    Get_MaxRepriceIterations()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PathNetwork_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 2; }
    return Settings->Get_MaxRepriceIterations();
}

// --------------------------------------------------------------------------------------------------------------------
