#pragma once


#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkEcs_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Ecs_ProcessorInjectors_PDA;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_HandleDebuggerBehavior : uint8
{
    Disable,
    Enable,

    // Stringify a list of all fragments and display it when hovering over a BP Entity/Handle
    EnableWithBlueprintDebugging
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ecs_HandleDebuggerBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_EntityMap_Policy : uint8
{
    DoNotLog,
    AlwaysLog
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ecs_EntityMap_Policy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "ECS"))
class CKECS_API UCk_Ecs_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Entity Script")
    FString _EntityScriptSpawnParamsFolderName = FString{TEXT("EntitySpawnParams")};

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS World",
              meta = (AllowPrivateAccess = true, AllowAbstract = false, MetaClass = "/Script/CkEcs.Ck_Ecs_ProcessorInjectors_PDA"))
    FSoftClassPath _ProcessorInjectors;

public:
    CK_PROPERTY_GET(_EntityScriptSpawnParamsFolderName);
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
    // This property can only be changed by the toolbar widgets
    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_HandleDebuggerBehavior _HandleDebuggerBehavior = ECk_Ecs_HandleDebuggerBehavior::Enable;

    // EntityMap helps us link up an Entity ID with its Actor/ConstructionScript/Ability by logging all Entities that are created
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    ECk_Ecs_EntityMap_Policy _EntityMapPolicy = ECk_Ecs_EntityMap_Policy::DoNotLog;

public:
    CK_PROPERTY(_HandleDebuggerBehavior);
    CK_PROPERTY(_EntityMapPolicy);
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
    static FString
    Get_EntityScriptSpawnParamsFolderName();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static ECk_Ecs_HandleDebuggerBehavior
    Get_HandleDebuggerBehavior();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_HandleDebuggerBehavior(
        ECk_Ecs_HandleDebuggerBehavior InHandleDebuggerBehavior);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static ECk_Ecs_EntityMap_Policy
    Get_EntityMapPolicy();

public:
    static auto Get_ProcessorInjectors() -> UCk_Ecs_ProcessorInjectors_PDA*;
};

// --------------------------------------------------------------------------------------------------------------------
