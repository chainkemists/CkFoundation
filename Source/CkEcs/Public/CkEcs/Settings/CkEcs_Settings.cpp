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
    Get_EntityScriptSpawnParamsPath() -> FDirectoryPath
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_EntityScriptSpawnParamsPath();
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

// --------------------------------------------------------------------------------------------------------------------