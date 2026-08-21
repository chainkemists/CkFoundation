#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkInput/Subsystem/CkKeyBinding_Subsystem.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <GameFramework/PlayerController.h>
#include <GameplayTagContainer.h>
#include <InputAction.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkKeyBinding_Utils.generated.h"

class UInputMappingContext;
class UPlayerMappableKeySettings;

// --------------------------------------------------------------------------------------------------------------------

/** Core info extracted from a UPlayerMappableKeySettings on an Input Action. */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_KeyBinding_MappableKeyInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_KeyBinding_MappableKeyInfo);

private:
    /** A unique name for this player mapping (e.g. "IA_Jump"). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _MappingName = NAME_None;

    /** The localized display name shown to the player (e.g. "Jump"). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FText _DisplayName = FText::GetEmpty();

    /** The category this mapping belongs to (e.g. "Movement"). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FText _DisplayCategory = FText::GetEmpty();

    /** Optional metadata object (icons, ability assets, etc.). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Metadata = nullptr;

public:
    CK_PROPERTY_GET(_MappingName);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_DisplayCategory);
    CK_PROPERTY(_Metadata);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_KeyBinding_MappableKeyInfo, _MappingName);
};

// --------------------------------------------------------------------------------------------------------------------

/** Controls how Get_HasKeyConflicts filters conflicts across categories. */
UENUM(BlueprintType)
enum class ECk_KeyConflictScope : uint8
{
    /** Flag any mapping that shares the key, regardless of category. */
    All,

    /** Only flag mappings whose DisplayCategory matches the source mapping's category.
     *  Compares localized DISPLAY text — self-consistent, but couples semantics to presentation;
     *  prefer SameScopeTags for anything beyond a single-category setup. */
    SameCategory,

    /** Only flag mappings whose UCk_PlayerMappableKeySettings_UE scope tags INTERSECT the source
     *  mapping's. The semantic scope: presentation renames cannot shift it, and tag hierarchy
     *  works (Input.Scope.Station.Roulette never collides with Input.Scope.Gameplay). An untagged
     *  mapping never matches — tag every mappable the game shows on its rebind screen. */
    SameScopeTags
};

// --------------------------------------------------------------------------------------------------------------------

/** Describes a single key binding conflict found by Get_HasKeyConflicts. */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_KeyBinding_ConflictInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_KeyBinding_ConflictInfo);

private:
    /** The internal mapping name (e.g. "IA_Jump") — use this to call SwapKeys / UnbindConflictAndRemap. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _MappingName = NAME_None;

    /** The player-facing display name (e.g. "Jump") — use this in the UI. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FText _DisplayName = FText::GetEmpty();

    /** The category this mapping belongs to (e.g. "Movement", "Combat"). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FText _DisplayCategory = FText::GetEmpty();

    /** The key that is currently bound to this conflicting mapping. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FKey _CurrentKey = FKey{EKeys::Invalid};

    /** Which slot the conflict is in (First, Second, etc.). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    EPlayerMappableKeySlot _Slot = EPlayerMappableKeySlot::Unspecified;

public:
    CK_PROPERTY_GET(_MappingName);
    CK_PROPERTY_GET(_DisplayName);
    CK_PROPERTY_GET(_DisplayCategory);
    CK_PROPERTY_GET(_CurrentKey);
    CK_PROPERTY_GET(_Slot);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_KeyBinding_ConflictInfo, _MappingName, _DisplayName, _DisplayCategory, _CurrentKey, _Slot);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Blueprint function library Enhanced Input User Settings API
 * for player key rebinding. Provides query, remap, conflict detection, and
 * persistence helpers designed for use from settings UI widgets.
 */
UCLASS(NotBlueprintable)
class CKINPUT_API UCk_Utils_KeyBinding_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_KeyBinding_UE);

public:

    // --- Core ---

    /**
     * Whether Enhanced Input user settings are enabled project-wide (Project Settings > Engine >
     * Enhanced Input > Enable User Settings). The ENGINE default is false, and while it is off the
     * engine never creates a key profile: every query, remap and key-changed delegate on this class
     * is permanently inert, and returns empty rather than failing. Gate a rebinding screen on this
     * rather than on an empty remappable-key list, which cannot tell "disabled" from "none authored".
     */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Are User Settings Enabled")
    static bool
    Get_AreUserSettingsEnabled();

    /** Get the Enhanced Input User Settings for the given player controller. */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Input User Settings")
    static UEnhancedInputUserSettings*
    Get_InputUserSettings(
        const APlayerController* InPlayerController);

    /**
     * Get all player-mappable key bindings from the active key profile.
     * Only Input Actions with a UPlayerMappableKeySettings are included.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get All Remappable Keys")
    static TArray<FPlayerKeyMapping>
    Get_AllRemappableKeys(
        const APlayerController* InPlayerController);

    /**
     * Get the key currently bound to a specific mapping name and slot.
     * Returns EKeys::Invalid if the mapping is not found.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Key For Mapping")
    static FKey
    Get_KeyForMapping(
        const APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot = EPlayerMappableKeySlot::First);

    /**
     * Get all mapping names currently bound to a given key.
     * Useful for discovering which actions share a key before batch-remapping.
     * @param InPlayerController  The player controller
     * @param InKey               The key to query
     * @return                    Array of mapping names bound to InKey
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Mapping Names For Key")
    static TArray<FName>
    Get_MappingNamesForKey(
        const APlayerController* InPlayerController,
        FKey InKey);

    /**
     * Get the key currently bound to an Input Action's mapping.
     * Shorthand for Get_MappingNameFromInputAction → Get_KeyForMapping.
     * Returns EKeys::Invalid if the Input Action has no mappable key settings or the mapping is not found.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Key For Input Action")
    static FKey
    Get_KeyForInputAction(
        const APlayerController* InPlayerController,
        const UInputAction* InInputAction,
        EPlayerMappableKeySlot InSlot = EPlayerMappableKeySlot::First);

    /**
     * Extract the mapping name from an Input Action's Player Mappable Key Settings.
     * Returns NAME_None if the Input Action is null or has no Player Mappable Key Settings.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Mapping Name From Input Action")
    static FName
    Get_MappingNameFromInputAction(
        const UInputAction* InInputAction);

    /**
     * Extract core Player Mappable Key Settings info from an Input Action.
     * @param InInputAction  The Input Action to extract from
     * @param OutInfo        The extracted mapping name, display name, category, and metadata
     * @return               True if the Input Action has valid Player Mappable Key Settings
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Mappable Key Info From Input Action")
    static bool
    Get_MappableKeyInfoFromInputAction(
        const UInputAction* InInputAction,
        FCk_KeyBinding_MappableKeyInfo& OutInfo);

    // --- Change Detection ---

    /**
     * Check if a specific mapping's key has changed compared to a cached value.
     * Call this from an OnSettingsChanged handler to filter for your mapping.
     * @param InPlayerController  The player controller
     * @param InMappingName       The mapping name to check
     * @param InSlot              Which slot to check
     * @param InCachedKey         The previously cached key value
     * @param OutCurrentKey       The current key (updated regardless of change)
     * @return                    True if the key differs from InCachedKey
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Did Mapping Key Change")
    static bool
    Get_DidMappingKeyChange(
        const APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InCachedKey,
        FKey& OutCurrentKey);

    /**
     * Start listening for changes on a specific mapping via the KeyBinding subsystem.
     * @param InPlayerController  The player controller (used to find the local player subsystem)
     * @param InMappingName       The mapping name to watch
     * @param InSlot              Which slot to watch
     * @param InOnChanged         Delegate receiving (MappingName, OldKey, NewKey)
     * @return                    Opaque handle — store it and pass to Unbind later
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Bind To Mapping Key Changed")
    static FCk_Handle_KeybindListener
    BindTo_OnMappingKeyChanged(
        const APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FCk_OnMappingKeyChanged InOnChanged);

    /**
     * Stop listening for changes on a specific mapping via the KeyBinding subsystem.
     * @param InPlayerController The player controller (used to find the local player subsystem)
     * @param InHandle  The handle returned by BindToMappingKeyChanged
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Unbind From Mapping Key Changed")
    static void
    UnbindFrom_OnMappingKeyChanged(
        const APlayerController* InPlayerController,
        UPARAM(ref) FCk_Handle_KeybindListener& InHandle);

    // --- Remapping ---

    /**
     * Rebind a key mapping to a new key.
     * @param InPlayerController  The player controller
     * @param InMappingName       Unique mapping name (e.g. "IA_Jump") from UPlayerMappableKeySettings
     * @param InSlot              Which slot to rebind (First, Second, etc.)
     * @param InNewKey            The new FKey to bind
     * @param OutFailureReason    Gameplay tags describing the failure (empty on success)
     * @return                    True if the rebind succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Remap Key")
    static bool
    RemapKey(
        const APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason);

    /**
     * Rebind multiple mappings to the same new key in a single operation.
     * Defers the OnSettingsChanged broadcast until the last remap so only one notification fires.
     * @param InPlayerController  The player controller
     * @param InMappingNames      Array of mapping names to rebind
     * @param InSlot              Which slot to rebind (First, Second, etc.)
     * @param InNewKey            The new FKey to bind
     * @param OutFailureReason    Gameplay tags describing the failure (empty on success)
     * @return                    True if all remaps succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Remap Keys", meta = (AutoCreateRefTerm = "InMappingNames"))
    static bool
    RemapKeys(
        const APlayerController* InPlayerController,
        const TArray<FName>& InMappingNames,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason);

    // --- Authoring (runtime-built contexts) ---

    /**
     * Marks ONE (action, key) mapping of a runtime-built context player-mappable under its own
     * name — the per-mapping override Enhanced Input uses to expose multiple rebind rows for a
     * single Input Action (each WASD direction of one Axis2d move action). Call BEFORE the
     * context is applied; registration reads the settings at apply time.
     *
     * Editor-authored contexts set this in the details panel; that authoring surface is
     * friend-gated in C++, so this is the code path for contexts built at runtime.
     *
     * @param InContext          The context that owns the mapping (typically still being built)
     * @param InAction           The mapped action
     * @param InKey              The mapped key identifying WHICH of the action's mappings
     * @param InMappingName      Unique mapping name the rebind system addresses (e.g. "IA_Move_Forward")
     * @param InDisplayName      Player-facing name for rebind rows
     * @param InDisplayCategory  Section header category ("Gameplay", "Roulette"); empty = uncategorized
     * @param InScopeTags        Semantic conflict scope (ECk_KeyConflictScope::SameScopeTags)
     * @return                   True if the mapping was found and marked
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Make Mapping Player Mappable",
              meta = (AutoCreateRefTerm = "InDisplayCategory,InScopeTags"))
    static bool
    MakeMappingPlayerMappable(
        UInputMappingContext* InContext,
        const UInputAction* InAction,
        FKey InKey,
        FName InMappingName,
        FText InDisplayName,
        const FText& InDisplayCategory,
        const FGameplayTagContainer& InScopeTags);

    /**
     * The mappable settings a mapping NAME resolves to, scanning the registered contexts —
     * reaches per-mapping override settings the profile's own rows cannot (the profile stores
     * copies of the display fields only). Null when no registered mapping carries settings under
     * that name.
     */
    static auto
    Get_MappableSettingsForMapping(
        const APlayerController* InPlayerController,
        FName InMappingName) -> const UPlayerMappableKeySettings*;

    // --- Reset ---

    /** Reset a single mapping row back to its default key(s). */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Reset Mapping To Default")
    static void
    ResetMappingToDefault(
        const APlayerController* InPlayerController,
        FName InMappingName);

    /** Reset ALL key mappings on the active profile to their defaults. */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Reset All Keys To Defaults")
    static void
    ResetAllToDefaults(
        const APlayerController* InPlayerController);

    // --- Persistence ---

    /** Persist the current key bindings to disk asynchronously. */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Save Key Bindings")
    static void
    SaveKeyBindings(
        const APlayerController* InPlayerController);

    // --- Conflict Detection ---

    /**
     * Check if a key is already bound to another action.
     * @param InPlayerController    The player controller
     * @param InNewKey              The key to check for conflicts
     * @param InExcludeMappingNames Mapping names to exclude (the group being rebound)
     * @param OutConflicts          Detailed info about each conflicting binding
     * @param InScope               All = flag every conflict; SameCategory = only flag conflicts
     *                              whose DisplayCategory matches the first excluded mapping's category
     * @return                      True if there are conflicts
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Has Key Conflicts", meta = (AutoCreateRefTerm = "InExcludeMappingNames"))
    static bool
    Get_HasKeyConflicts(
        const APlayerController* InPlayerController,
        FKey InNewKey,
        const TArray<FName>& InExcludeMappingNames,
        TArray<FCk_KeyBinding_ConflictInfo>& OutConflicts,
        ECk_KeyConflictScope InScope = ECk_KeyConflictScope::All);

    // --- Conflict Resolution ---

    /**
     * Swap two key bindings. Assigns InNewKey to InMappingName and moves
     * InMappingName's old key to whatever action currently holds InNewKey.
     * @param OutFailureReason  Gameplay tags describing the failure (empty on success)
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Swap Keys")
    static bool
    SwapKeys(
        const APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason);

    /**
     * Unbind every conflicting holder of InNewKey — their mapping becomes UNBOUND, not
     * reset to its default — then map InMappingName to it.
     * @param OutFailureReason  Gameplay tags describing the failure (empty on success)
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Unbind Conflict And Remap")
    static bool
    UnbindConflictAndRemap(
        const APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason);
};

// --------------------------------------------------------------------------------------------------------------------
