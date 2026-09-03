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

auto
    UCk_Utils_Nav_Settings_UE::
    Get_NavQueryVerticalHalfExtent()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return -1.0f; }
    return Settings->Get_NavQueryVerticalHalfExtent();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_Settings_UE::
    Get_NavQueryProjectionExtentVec()
    -> FVector
{
    const auto HExt    = Get_NavQuerySearchHalfExtent();
    const auto VExtRaw = Get_NavQueryVerticalHalfExtent();
    const auto VExt    = VExtRaw < 0.0f ? HExt : VExtRaw;
    return FVector{HExt, HExt, VExt};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_Settings_UE::
    Get_DefaultNavSurfaceProvider()
    -> ECk_NavSurface_Provider
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return ECk_NavSurface_Provider::Recast; }
    return Settings->Get_DefaultNavSurfaceProvider();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Nav_Settings_UE::
    Get_DefaultNavSurfaceShadowMode()
    -> ECk_NavSurface_ShadowMode
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Nav_ProjectSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return ECk_NavSurface_ShadowMode::Off; }
    return Settings->Get_DefaultNavSurfaceShadowMode();
}

// --------------------------------------------------------------------------------------------------------------------
