#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

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

USTRUCT(BlueprintType)
struct CKECS_API FCk_Ecs_ProcessorInjectors_Info
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ecs_ProcessorInjectors_Info);

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    TEnumAsByte<ETickingGroup> _TickingGroup = TG_PrePhysics;

    UPROPERTY(EditDefaultsOnly, Instanced, meta=(AllowPrivateAccess))
    TArray<TObjectPtr<class UCk_EcsWorld_ProcessorInjector_Base_UE>>  _ProcessorInjectors;

public:
    CK_PROPERTY(_TickingGroup);
    CK_PROPERTY_GET(_ProcessorInjectors);

    CK_DEFINE_CONSTRUCTORS(FCk_Ecs_ProcessorInjectors_Info, _ProcessorInjectors);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable)
class CKECS_API UCk_Ecs_ProcessorInjectors_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ProcessorInjectors_PDA);

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    TArray<FCk_Ecs_ProcessorInjectors_Info> _ProcessorInjectors;

public:
    CK_PROPERTY_GET(_ProcessorInjectors);
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
              meta = (AllowPrivateAccess = true, AllowAbstract = false, MetaClass = "/Script/CkEcs.Ck_Ecs_ProcessorInjectors_PDA"))
    FSoftClassPath _ProcessorInjectors;

    // Enabling this will slow down the game's execution
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_HandleDebuggerBehavior _HandleDebuggerBehavior = ECk_Ecs_HandleDebuggerBehavior::Disable;

public:
    CK_PROPERTY_GET(_ProcessorInjectors);
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
    static auto Get_ProcessorInjectors() -> UCk_Ecs_ProcessorInjectors_PDA*;
};

// --------------------------------------------------------------------------------------------------------------------
