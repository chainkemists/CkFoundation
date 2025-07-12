#include "CkCore_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

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

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_MaxNumberOfBlueprintStackFrames()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_MaxNumberOfBlueprintStackFrames();
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_EnsureBreakInBlueprintsPolicy()
    -> ECk_EnsureBreakInBlueprints_Policy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_EnsureBreakInBlueprintsPolicy();
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_EnsureBreakInAngelscriptPolicy()
    -> ECk_EnsureBreakInAngelscript_Policy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_EnsureBreakInAngelscriptPolicy();
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_MaxNumberOfAngelscriptStackFrames()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_MaxNumberOfAngelscriptStackFrames();
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_EnsureDisplayPolicy()
    -> ECk_EnsureDisplay_Policy
{
#if WITH_EDITOR
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_EnsureDisplayPolicy();
#else
    return ECk_EnsureDisplay_Policy::LogOnly;
#endif
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Get_EnsureDetailsPolicy()
    -> ECk_EnsureDetails_Policy
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Get_EnsureDetailsPolicy();
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Set_DefaultDebugNameVerbosity(
        [[maybe_unused]]
        ECk_DebugNameVerbosity_Policy InNewPolicy)
    -> void
{
#if NOT CK_DEBUG_NAME_FORCE_VERBOSE
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Set_DefaultDebugNameVerbosity(InNewPolicy);
#endif
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Set_EnsureBreakInBlueprintsPolicy(
        ECk_EnsureBreakInBlueprints_Policy InNewPolicy)
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Set_EnsureBreakInBlueprintsPolicy(InNewPolicy);
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Set_EnsureBreakInAngelscriptPolicy(
        ECk_EnsureBreakInAngelscript_Policy InNewPolicy)
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Set_EnsureBreakInAngelscriptPolicy(InNewPolicy);
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Set_EnsureDisplayPolicy(
        [[maybe_unused]]
        ECk_EnsureDisplay_Policy InNewPolicy)
    -> void
{
#if WITH_EDITOR
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Set_EnsureDisplayPolicy(InNewPolicy);
#endif
}

auto
    UCk_Utils_Core_UserSettings_UE::
    Set_EnsureDetailsPolicy(
        ECk_EnsureDetails_Policy InNewPolicy)
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>()->Set_EnsureDetailsPolicy(InNewPolicy);
}

// --------------------------------------------------------------------------------------------------------------------