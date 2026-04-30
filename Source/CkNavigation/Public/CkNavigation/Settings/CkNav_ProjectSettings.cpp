#include "CkNav_ProjectSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_Settings_UE::
    Get_MaxPathQueriesPerFrame()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 8; }
    return Settings->Get_MaxPathQueriesPerFrame();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_Settings_UE::
    Get_NavQuerySearchHalfExtent()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 500.0f; }
    return Settings->Get_NavQuerySearchHalfExtent();
}

// --------------------------------------------------------------------------------------------------------------------
