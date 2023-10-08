#include "CkEcs_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_EcsWorldActorClass()
    -> TSubclassOf<ACk_World_Actor_Base_UE>
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_EcsWorldActorClass().LoadSynchronous();
}

auto
    UCk_Utils_Ecs_ProjectSettings_UE::
    Get_EcsWorldTickingGroup()
    -> ETickingGroup
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Ecs_ProjectSettings_UE>()->Get_EcsWorldTickingGroup();
}

// --------------------------------------------------------------------------------------------------------------------
