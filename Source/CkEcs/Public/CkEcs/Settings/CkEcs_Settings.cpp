#include "CkEcs_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_ProcessorInjector()
    -> TSubclassOf<UCk_EcsWorld_ProcessorInjector_Base>
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_ProcessorInjector().LoadSynchronous();
}

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_EcsWorldTickingGroup()
    -> ETickingGroup
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_EcsWorldTickingGroup();
}

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_HandleDebuggerBehavior()
    -> ECk_Ecs_HandleDebuggerBehavior
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_HandleDebuggerBehavior();
}

// --------------------------------------------------------------------------------------------------------------------
