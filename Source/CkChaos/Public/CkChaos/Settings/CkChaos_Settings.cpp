#include "CkChaos_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_Chaos_Settings_UE::
    Get_MaximumRadialDamageDeltaRadiusPerFrame()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Chaos_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 100.0f; }

    return Settings->Get_MaximumRadialDamageDeltaRadiusPerFrame();
}
