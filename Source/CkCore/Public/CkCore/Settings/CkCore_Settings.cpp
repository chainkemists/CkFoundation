#include "CkCore_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Core_ProjectSettings_UE::
    Get_EnsureDisplayPolicy()
    -> ECk_EnsureDisplay_Policy
{
#if WITH_EDITOR
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_ProjectSettings_UE>()->Get_EnsureDisplayPolicy();
#else
    return ECk_EnsureDisplay_Policy::LogOnly;
#endif
}

auto
    UCk_Utils_Core_ProjectSettings_UE::
    Get_EnsureDetailsPolicy()
    -> ECk_EnsureDetails_Policy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_ProjectSettings_UE>()->Get_EnsureDetailsPolicy();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_DefaultDebugNameVerbosity()
    -> ECk_DebugNameVerbosity_Policy
{
#if NOT CK_DEBUG_NAME_FORCE_VERBOSE
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_DefaultDebugNameVerbosity();
#else
    return ECk_DebugNameVerbosity_Policy::Verbose;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
