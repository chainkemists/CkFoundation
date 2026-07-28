#include "CkResourceLoader_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_Settings_UE::
    Get_MaxNumberOfCachedResourcesPerType()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoader_ProjectSettings_UE>()->Get_MaxNumberOfCachedResourcesPerType();
}

auto
    UCk_Utils_ResourceLoader_Settings_UE::
    Get_DefaultLoadingPolicy()
    -> ECk_ResourceLoader_LoadingPolicy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoader_ProjectSettings_UE>()->Get_DefaultLoadingPolicy();
}

auto
    UCk_Utils_ResourceLoader_Settings_UE::
    Get_LoadingPolicyForConsumer(
        FName InConsumerId)
    -> ECk_ResourceLoader_LoadingPolicy
{
    const auto& Overrides = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoader_ProjectSettings_UE>()->Get_PerConsumerLoadingPolicyOverrides();

    if (const auto* Override = Overrides.Find(InConsumerId))
    { return *Override; }

    return Get_DefaultLoadingPolicy();
}

// --------------------------------------------------------------------------------------------------------------------
