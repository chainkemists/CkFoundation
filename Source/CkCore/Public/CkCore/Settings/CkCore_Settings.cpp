#include "CkCore_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Core_ProjectSettings_UE::
    Get_EnsureDisplayPolicy()
    -> ECk_Core_EnsureDisplayPolicy
{
#if WITH_EDITOR
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_ProjectSettings_UE>()->Get_EnsureDisplayPolicy();
#else
    return ECk_Core_EnsureDisplayPolicy::LogOnly;
#endif
}

auto
    UCk_Utils_Core_ProjectSettings_UE::
    Get_EnsureDetailsPolicy()
    -> ECk_Core_EnsureDetailsPolicy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_ProjectSettings_UE>()->Get_EnsureDetailsPolicy();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Core_UserSettings_UE::Get_DefaultDebugNameVerbosity()
        -> ECk_Core_DefaultDebugNameVerbosityPolicy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_DefaultDebugNameVerbosity();
}

// --------------------------------------------------------------------------------------------------------------------
