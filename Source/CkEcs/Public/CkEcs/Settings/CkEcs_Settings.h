#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkEcs_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_HandleDebuggerBehavior : uint8
{
    Disable,
    Enable,

    // Stringify a list of all fragments and display it when hovering over a BP Entity/Handle
    EnableWithBlueprintDebugging
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
    TSoftClassPtr<class UCk_EcsWorld_ProcessorInjector_Base> _ProcessorInjector;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS World",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<ETickingGroup> _EcsWorldTickingGroup = TG_PrePhysics;

    // Enabling this will slow down the game's execution
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_HandleDebuggerBehavior _HandleDebuggerBehavior = ECk_Ecs_HandleDebuggerBehavior::Disable;

public:
    CK_PROPERTY_GET(_ProcessorInjector);
    CK_PROPERTY_GET(_EcsWorldTickingGroup);
    CK_PROPERTY_GET(_HandleDebuggerBehavior);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_Utils_Ecs_ProjectSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ecs_ProjectSettings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static ECk_Ecs_HandleDebuggerBehavior
    Get_HandleDebuggerBehavior();

public:
    static auto Get_ProcessorInjector() -> TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base>;
    static auto Get_EcsWorldTickingGroup() -> ETickingGroup;
};

// --------------------------------------------------------------------------------------------------------------------
