#include "CkEqs_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_Settings_UE::
    Get_MaxCandidatesPerFrame()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Eqs_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 256; }
    return Settings->Get_MaxCandidatesPerFrame();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_Settings_UE::
    Get_MaxCandidates_ImmediatePathHardCap()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Eqs_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 1024; }
    return Settings->Get_MaxCandidates_ImmediatePathHardCap();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Eqs_Settings_UE::
    Get_DefaultPathCostAgentRadius()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Eqs_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 0.0f; }
    return Settings->Get_DefaultPathCostAgentRadius();
}

// --------------------------------------------------------------------------------------------------------------------
