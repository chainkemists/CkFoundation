#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Registry/CkRegistry_SlotTable.h"

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

// How the slot-table soft-warning surfaces when the active-registry count crosses a configured threshold.
// The hard cap (kRegistryTable_MaxSlots) is a compile-time invariant and always uses CK_ENSURE_IF_NOT;
// this only controls the soft-warning layer.
UENUM(BlueprintType)
enum class ECk_Ecs_RegistrySlot_Reporting : uint8
{
    Silent,
    Log,
    Warning,
    Ensure
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ecs_RegistrySlot_Reporting);

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

    // When enabled, the scheduler's pump pass short-circuits dirty-marker checks by comparing a per-fragment
    // version counter (bumped on every registry mutation) against a per-node cache that persists across
    // frames. Nodes whose marker has not changed since the last observation skip the Has_AnyEntityWith scan
    // entirely — load-bearing because that scan reports a tombstone-only (in_place_delete) marker pool as
    // non-empty forever after first use, which would otherwise phantom-pump every ever-used marker processor
    // every frame.
    //
    // Independent of this setting, a pump that provably visited zero entities does not count as work when
    // scheduling further pump passes (see FProcessorScheduler::DoPump).
    //
    // Cached at FProcessorScheduler construction — changing this requires a graph rebuild (PIE restart) to
    // take effect.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Scheduler")
    bool _EnableDirtyMarkerPumpShortCircuit = true;

    // When enabled, the scheduler's MAIN pass skips dispatching any eligible processor whose view is provably
    // empty — some required (include) fragment/tag type of its view has zero live entities — bypassing the
    // Tick call, view construction, tombstone walk, and per-processor trace/stat/debug overhead entirely.
    // Emptiness is tracked with the same per-fragment version counters the pump short-circuit uses: the
    // (tombstone-aware) storage scan re-runs only when some include type mutated since the last observation.
    //
    // Eligibility is conservative and automatic — only processors whose DoTick is the template-generated view
    // iteration participate; custom-DoTick processors, composites, parallel processors, and script processors
    // always tick. Per-processor opt-out: ECk_ProcessorEmptyViewPolicy::AlwaysTick. Dev-build cross-check:
    // `ck.Scheduler.VerifyEmptyViewSkip 1` re-scans every skipped node and ensures the verdict still holds.
    //
    // Cached at FProcessorScheduler construction — changing this requires a graph rebuild (PIE restart) to
    // take effect.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Scheduler")
    bool _EnableEmptyViewMainPassSkip = true;

    // Active-registry count at which the slot table fires a soft-warning. Fires once at this threshold,
    // then again at each new ascending multiple (2x, 3x, ...). Never re-fires for a multiple already
    // reported in this process lifetime. 0 disables the soft-warning entirely (hard cap still applies).
    //
    // If you have tests or gyms that intentionally exceed this number, either raise the threshold or
    // set Reporting to Log/Silent so the AutoTest harness doesn't escalate the signal to a test failure.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Registry Slot Table",
              meta = (ClampMin = "0"))
    int32 _RegistrySlot_WarnThreshold = 1024;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Registry Slot Table",
              meta = (EditCondition = "_RegistrySlot_WarnThreshold > 0"))
    ECk_Ecs_RegistrySlot_Reporting _RegistrySlot_WarnReporting = ECk_Ecs_RegistrySlot_Reporting::Ensure;

    // Read-only display of the compile-time hard cap. Editing this in the .ini does nothing —
    // the canonical value is ck::registry_table::kRegistryTable_MaxSlots in
    // CkRegistry_SlotTable.h. Exposed here so it's discoverable from Project Settings and
    // queryable at runtime (e.g. by CkWatermark) via Get_RegistrySlot_HardCap().
    UPROPERTY(VisibleAnywhere, Category = "Registry Slot Table",
              meta = (DisplayName = "Hard Cap (read-only)"))
    int32 _RegistrySlot_HardCap = ck::registry_table::kRegistryTable_MaxSlots;

public:
    CK_PROPERTY_GET(_EntityScriptSpawnParamsFolderName);
    CK_PROPERTY_GET(_IgnoredSpawnParamsPropertyNames);
    CK_PROPERTY_GET(_EnableDirtyMarkerPumpShortCircuit);
    CK_PROPERTY_GET(_EnableEmptyViewMainPassSkip);
    CK_PROPERTY_GET(_RegistrySlot_WarnThreshold);
    CK_PROPERTY_GET(_RegistrySlot_WarnReporting);
    CK_PROPERTY_GET(_RegistrySlot_HardCap);
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
    static bool
    Get_EnableDirtyMarkerPumpShortCircuit();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static bool
    Get_EnableEmptyViewMainPassSkip();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static int32
    Get_RegistrySlot_WarnThreshold();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static ECk_Ecs_RegistrySlot_Reporting
    Get_RegistrySlot_WarnReporting();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ecs|Settings")
    static int32
    Get_RegistrySlot_HardCap();

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
