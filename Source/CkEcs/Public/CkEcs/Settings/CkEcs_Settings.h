#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkEcs_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_HandleDebuggerBehavior : uint8
{
    Disable,
    Enable
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "ECS"))
class CKECS_API UCk_Ecs_ProjectSettings_UE : public UCk_Engine_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS World",
              meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSoftClassPtr<class ACk_World_Actor_Base_UE> _EcsWorldActorClass;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS World",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<ETickingGroup> _EcsWorldTickingGroup = TG_PrePhysics;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_HandleDebuggerBehavior _HandleDebuggerBehavior = ECk_Ecs_HandleDebuggerBehavior::Disable;

public:
    CK_PROPERTY_GET(_EcsWorldActorClass);
    CK_PROPERTY_GET(_EcsWorldTickingGroup);
    CK_PROPERTY_GET(_HandleDebuggerBehavior);
};

// --------------------------------------------------------------------------------------------------------------------

class CKECS_API UCk_Utils_Ecs_ProjectSettings_UE
{
public:
    static auto Get_EcsWorldActorClass() -> TSubclassOf<class ACk_World_Actor_Base_UE>;
    static auto Get_EcsWorldTickingGroup() -> ETickingGroup;
    static auto Get_HandleDebuggerBehavior() -> ECk_Ecs_HandleDebuggerBehavior;
};

// --------------------------------------------------------------------------------------------------------------------
