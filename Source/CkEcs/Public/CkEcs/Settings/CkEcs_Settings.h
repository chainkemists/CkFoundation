#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkEcs_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

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

    UPROPERTY(Config, EditDefaultsOnly, Category = "Entity Script",
              meta = (ToolTip = "Property names to hide from Cue K2Node pins and SpawnParams Toolbox comparisons (e.g. 'Dummy', 'Placeholder'). 'MemberVar_0' (bool) is always ignored as it is the dummy variable required by empty Blueprint structs."))
    TArray<FString> _IgnoredSpawnParamsPropertyNames;

public:
    CK_PROPERTY_GET(_EntityScriptSpawnParamsFolderName);
    CK_PROPERTY_GET(_IgnoredSpawnParamsPropertyNames);
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

    // Debug Callstack Capture Settings
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true))
    bool _CaptureCallstack_Cpp = false;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true))
    bool _CaptureCallstack_Blueprint = false;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true))
    bool _CaptureCallstack_Angelscript = false;

    // Maximum number of stack frames to capture for C++ callstacks
    // Lower values = faster capture (address-only capture is ~1-5μs per callstack)
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "128"))
    int32 _MaxCallstackFrames_Cpp = 8;

    // Blueprint/Angelscript use max frames from Core user settings by default
    // These overrides only apply when the corresponding CaptureCallstack flag is enabled
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "128",
                     EditCondition = "_CaptureCallstack_Blueprint"))
    int32 _MaxCallstackFrames_Blueprint_Override = 0;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "128",
                     EditCondition = "_CaptureCallstack_Angelscript"))
    int32 _MaxCallstackFrames_Angelscript_Override = 0;

    // Maximum number of callstack entries to keep per entity
    // When limit is reached, oldest entries are removed (FIFO)
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Debug Callstack",
              meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "1024"))
    int32 _MaxCallstackEntries = 8;

public:
    CK_PROPERTY(_HandleDebuggerBehavior);
    CK_PROPERTY(_EntityMapPolicy);
    CK_PROPERTY(_CaptureCallstack_Cpp);
    CK_PROPERTY(_CaptureCallstack_Blueprint);
    CK_PROPERTY(_CaptureCallstack_Angelscript);
    CK_PROPERTY(_MaxCallstackFrames_Cpp);
    CK_PROPERTY(_MaxCallstackFrames_Blueprint_Override);
    CK_PROPERTY(_MaxCallstackFrames_Angelscript_Override);
    CK_PROPERTY(_MaxCallstackEntries);
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
    static TArray<FString>
    Get_IgnoredSpawnParamsPropertyNames();

    static bool
    Is_IgnoredSpawnParamsProperty(const FString& InPropertyName);

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

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static bool
    Get_CaptureCallstack_Cpp();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_CaptureCallstack_Cpp(bool InEnabled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static bool
    Get_CaptureCallstack_Blueprint();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_CaptureCallstack_Blueprint(bool InEnabled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static bool
    Get_CaptureCallstack_Angelscript();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_CaptureCallstack_Angelscript(bool InEnabled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static int32
    Get_MaxCallstackFrames_Cpp();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_MaxCallstackFrames_Cpp(int32 InMaxFrames);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static int32
    Get_MaxCallstackFrames_Blueprint_Override();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_MaxCallstackFrames_Blueprint_Override(int32 InMaxFrames);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static int32
    Get_MaxCallstackFrames_Angelscript_Override();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_MaxCallstackFrames_Angelscript_Override(int32 InMaxFrames);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static int32
    Get_MaxCallstackEntries();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ecs|Settings")
    static void
    Set_MaxCallstackEntries(int32 InMaxEntries);

public:
};

// --------------------------------------------------------------------------------------------------------------------
