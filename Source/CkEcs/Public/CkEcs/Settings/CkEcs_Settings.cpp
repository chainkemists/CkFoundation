#include "CkEcs_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_ProcessorInjectors()
    -> UCk_Ecs_ProcessorInjectors_PDA*
{
    return
        UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProcessorInjectors_PDA>(
            UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_ProcessorInjectors());
}

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_HandleDebuggerBehavior()
    -> ECk_Ecs_HandleDebuggerBehavior
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_HandleDebuggerBehavior();
}

// --------------------------------------------------------------------------------------------------------------------
