#include "CkEcs_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/ProcessorInjector/CkEcsMetaProcessorInjector.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_ProcessorInjectors()
    -> UCk_Ecs_ProcessorInjectors_PDA*
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>();
    const auto ProcessorInjectors = Settings->Get_ProcessorInjectors().TryLoadClass<UCk_Ecs_ProcessorInjectors_PDA>();

    CK_ENSURE_IF_NOT(ck::IsValid(ProcessorInjectors),
        TEXT("Could not load ProcessorInjectors from [{}] defined in project settings"), ProcessorInjectors)
    { return {}; }

    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProcessorInjectors_PDA>(ProcessorInjectors);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_EntityScriptSpawnParamsFolderName()
    -> FString
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_EntityScriptSpawnParamsFolderName();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_HandleDebuggerBehavior()
    -> ECk_Ecs_HandleDebuggerBehavior
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_Ecs_HandleDebuggerBehavior::Disable; }

    return Settings->Get_HandleDebuggerBehavior();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_HandleDebuggerBehavior(
        ECk_Ecs_HandleDebuggerBehavior InHandleDebuggerBehavior)
    -> void
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_HandleDebuggerBehavior(InHandleDebuggerBehavior);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_EntityMapPolicy()
    -> ECk_Ecs_EntityMap_Policy
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_Ecs_EntityMap_Policy::DoNotLog; }

    return Settings->Get_EntityMapPolicy();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_CaptureCallstack_Cpp()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_CaptureCallstack_Cpp();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_CaptureCallstack_Cpp(bool InEnabled)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_CaptureCallstack_Cpp(InEnabled);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_CaptureCallstack_Blueprint()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_CaptureCallstack_Blueprint();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_CaptureCallstack_Blueprint(bool InEnabled)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_CaptureCallstack_Blueprint(InEnabled);
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Get_CaptureCallstack_Angelscript()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    return Settings->Get_CaptureCallstack_Angelscript();
}

auto
    UCk_Utils_Ecs_Settings_UE::
    Set_CaptureCallstack_Angelscript(bool InEnabled)
    -> void
{
    auto Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->Set_CaptureCallstack_Angelscript(InEnabled);
}

// --------------------------------------------------------------------------------------------------------------------