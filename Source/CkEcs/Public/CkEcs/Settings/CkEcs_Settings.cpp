#include "CkEcs_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

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
    Get_HandleDebuggerBehavior()
    -> ECk_Ecs_HandleDebuggerBehavior
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_UserSettings_UE>()->Get_HandleDebuggerBehavior();
}

// --------------------------------------------------------------------------------------------------------------------
