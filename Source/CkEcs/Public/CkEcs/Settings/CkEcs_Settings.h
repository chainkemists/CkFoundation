#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkSettings/UserSettings/CkUserSettings.h"

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

UENUM(BlueprintType)
enum class ECk_Ecs_EntityMap_Policy : uint8
{
    DoNotLog,
    AlwaysLog
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Ecs_ProcessorInjectors_Info
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ecs_ProcessorInjectors_Info);

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    FName _Description = NAME_None;
#endif

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    TEnumAsByte<ETickingGroup> _TickingGroup = TG_PrePhysics;

    // Processors can be pumped multiple times _if_ they have requests that still need to be processed
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, ClampMin="0", UIMin="0"))
    int32 _MaximumNumberOfPumps = 1;

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    TArray<TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>>  _ProcessorInjectors;

public:
    CK_PROPERTY(_TickingGroup);
    CK_PROPERTY_GET(_MaximumNumberOfPumps);
    CK_PROPERTY_GET(_ProcessorInjectors);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable)
class CKECS_API UCk_Ecs_ProcessorInjectors_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ProcessorInjectors_PDA);

private:
    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess, TitleProperty = "_Description"))
    TArray<FCk_Ecs_ProcessorInjectors_Info> _ProcessorInjectors;

public:
    CK_PROPERTY_GET(_ProcessorInjectors);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "ECS"))
class CKECS_API UCk_Ecs_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS World",
              meta = (AllowPrivateAccess = true, AllowAbstract = false, MetaClass = "/Script/CkEcs.Ck_Ecs_ProcessorInjectors_PDA"))
    FSoftClassPath _ProcessorInjectors;

public:
    CK_PROPERTY_GET(_ProcessorInjectors);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "ECS"))
class CKECS_API UCk_Ecs_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_UserSettings_UE);

private:
    // Enabling this will slow down the game's execution
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_HandleDebuggerBehavior _HandleDebuggerBehavior = ECk_Ecs_HandleDebuggerBehavior::Disable;

    // EntityMap helps us link up an Entity ID with its Actor/ConstructionScript/Ability by logging all Entities that are created
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_EntityMap_Policy _EntityMapPolicy = ECk_Ecs_EntityMap_Policy::DoNotLog;

public:
    CK_PROPERTY_GET(_HandleDebuggerBehavior);
    CK_PROPERTY_GET(_EntityMapPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_Utils_Ecs_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ecs_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static ECk_Ecs_HandleDebuggerBehavior
    Get_HandleDebuggerBehavior();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static ECk_Ecs_EntityMap_Policy
    Get_EntityMapPolicy();

public:
    static auto Get_ProcessorInjectors() -> UCk_Ecs_ProcessorInjectors_PDA*;
};

// --------------------------------------------------------------------------------------------------------------------
