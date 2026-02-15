#include "CkCore_Settings.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_core_settings
{
    // CVar for DefaultDebugNameVerbosity
    static TAutoConsoleVariable<int32> CVar_DefaultDebugNameVerbosity(
        TEXT("ck.ensure.DebugNameVerbosity"),
        -1,
        TEXT("Set default debug name verbosity policy\n")
        TEXT("0 = Compact, 1 = Verbose\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Policy = static_cast<ECk_DebugNameVerbosity_Policy>(Value);
                UCk_Utils_Core_UserSettings_UE::Set_DefaultDebugNameVerbosity(Policy);
                ck::core::Log(TEXT("Debug name verbosity set to: [{}]"), Policy);
            }
        }),
        ECVF_Default);

    // CVar for EnsureDisplayPolicy
    static TAutoConsoleVariable<int32> CVar_EnsureDisplayPolicy(
        TEXT("ck.ensure.DisplayPolicy"),
        -1,
        TEXT("Set ensure display policy\n")
        TEXT("0 = ModalDialog, 1 = MessageLog, 2 = StreamerMode, 3 = LogOnly\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Policy = static_cast<ECk_EnsureDisplay_Policy>(Value);
                UCk_Utils_Core_UserSettings_UE::Set_EnsureDisplayPolicy(Policy);
                ck::core::Log(TEXT("Ensure display policy set to: [{}]"), Policy);
            }
        }),
        ECVF_Default);

    // CVar for EnsureDetailsPolicy
    static TAutoConsoleVariable<int32> CVar_EnsureDetailsPolicy(
        TEXT("ck.ensure.DetailsPolicy"),
        -1,
        TEXT("Set ensure details policy\n")
        TEXT("0 = MessageOnly, 1 = MessageAndStackTrace\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Policy = static_cast<ECk_EnsureDetails_Policy>(Value);
                UCk_Utils_Core_UserSettings_UE::Set_EnsureDetailsPolicy(Policy);
                ck::core::Log(TEXT("Ensure details policy set to: [{}]"), Policy);
            }
        }),
        ECVF_Default);

    // CVar for EnsureBreakInBlueprintsPolicy
    static TAutoConsoleVariable<int32> CVar_EnsureBreakInBlueprintsPolicy(
        TEXT("ck.ensure.BreakInBlueprints"),
        -1,
        TEXT("Set ensure break in Blueprints policy\n")
        TEXT("0 = AlwaysBreak, 1 = UseUnrealExceptionBehavior\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Policy = static_cast<ECk_EnsureBreakInBlueprints_Policy>(Value);
                UCk_Utils_Core_UserSettings_UE::Set_EnsureBreakInBlueprintsPolicy(Policy);
                ck::core::Log(TEXT("Ensure break in Blueprints policy set to: [{}]"), Policy);
            }
        }),
        ECVF_Default);

    // CVar for EnsureBreakInAngelscriptPolicy
    static TAutoConsoleVariable<int32> CVar_EnsureBreakInAngelscriptPolicy(
        TEXT("ck.ensure.BreakInAngelscript"),
        -1,
        TEXT("Set ensure break in Angelscript policy\n")
        TEXT("0 = NeverBreak, 1 = AlwaysBreak\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Policy = static_cast<ECk_EnsureBreakInAngelscript_Policy>(Value);
                UCk_Utils_Core_UserSettings_UE::Set_EnsureBreakInAngelscriptPolicy(Policy);
                ck::core::Log(TEXT("Ensure break in Angelscript policy set to: [{}]"), Policy);
            }
        }),
        ECVF_Default);

    // CVar for MaxNumberOfBlueprintStackFrames
    static TAutoConsoleVariable<int32> CVar_MaxBlueprintStackFrames(
        TEXT("ck.ensure.MaxBlueprintStackFrames"),
        -1,
        TEXT("Set maximum number of Blueprint stack frames to display\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>();
                Settings->Set_MaxNumberOfBlueprintStackFrames(Value);
                ck::core::Log(TEXT("Max Blueprint stack frames set to: [{}]"), Value);
            }
        }),
        ECVF_Default);

    // CVar for MaxNumberOfAngelscriptStackFrames
    static TAutoConsoleVariable<int32> CVar_MaxAngelscriptStackFrames(
        TEXT("ck.ensure.MaxAngelscriptStackFrames"),
        -1,
        TEXT("Set maximum number of Angelscript stack frames to display\n")
        TEXT("-1 = No change"),
        FConsoleVariableDelegate::CreateLambda([](const IConsoleVariable* Variable)
        {
            if (auto Value = Variable->GetInt();
                Value >= 0)
            {
                auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Core_UserSettings_UE>();
                Settings->Set_MaxNumberOfAngelscriptStackFrames(Value);
                ck::core::Log(TEXT("Max Angelscript stack frames set to: [{}]"), Value);
            }
        }),
        ECVF_Default);
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
