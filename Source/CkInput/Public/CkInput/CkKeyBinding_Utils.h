#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkInput/Subsystem/CkKeyBinding_Subsystem.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <GameFramework/PlayerController.h>
#include <InputAction.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkKeyBinding_Utils.generated.h"

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

    /** Only flag mappings whose DisplayCategory matches the source mapping's category. */
    SameCategory
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

    /** Get the Enhanced Input User Settings for the given player controller. */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Input User Settings")
    static UEnhancedInputUserSettings*
    Get_InputUserSettings(
        APlayerController* InPlayerController);

    /**
     * Get all player-mappable key bindings from the active key profile.
     * Only Input Actions with a UPlayerMappableKeySettings are included.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get All Remappable Keys")
    static TArray<FPlayerKeyMapping>
    Get_AllRemappableKeys(
        APlayerController* InPlayerController);

    /**
     * Get the key currently bound to a specific mapping name and slot.
     * Returns EKeys::Invalid if the mapping is not found.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Key For Mapping")
    static FKey
    Get_KeyForMapping(
        APlayerController* InPlayerController,
        FName InMappingName,
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
        APlayerController* InPlayerController,
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
        APlayerController* InPlayerController,
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
        APlayerController* InPlayerController,
        UPARAM(ref) FCk_Handle_KeybindListener& InHandle);

    // --- Remapping ---

    /**
     * Rebind a key mapping to a new key.
     * @param InPlayerController  The player controller
     * @param InMappingName       Unique mapping name (e.g. "IA_Jump") from UPlayerMappableKeySettings
     * @param InSlot              Which slot to rebind (First, Second, etc.)
     * @param InNewKey            The new FKey to bind
     * @return                    True if the rebind succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Remap Key")
    static bool
    RemapKey(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey);

    // --- Reset ---

    /** Reset a single mapping row back to its default key(s). */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Reset Mapping To Default")
    static void
    ResetMappingToDefault(
        APlayerController* InPlayerController,
        FName InMappingName);

    /** Reset ALL key mappings on the active profile to their defaults. */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Reset All Keys To Defaults")
    static void
    ResetAllToDefaults(
        APlayerController* InPlayerController);

    // --- Persistence ---

    /** Persist the current key bindings to disk asynchronously. */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Save Key Bindings")
    static void
    SaveKeyBindings(
        APlayerController* InPlayerController);

    // --- Conflict Detection ---

    /**
     * Check if a key is already bound to another action.
     * @param InPlayerController   The player controller
     * @param InNewKey             The key to check for conflicts
     * @param InExcludeMappingName The mapping being rebound (excluded from results)
     * @param OutConflicts         Detailed info about each conflicting binding
     * @param InScope              All = flag every conflict; SameCategory = only flag conflicts
     *                             whose DisplayCategory matches InExcludeMappingName's category
     * @return                     True if there are conflicts
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Has Key Conflicts")
    static bool
    Get_HasKeyConflicts(
        APlayerController* InPlayerController,
        FKey InNewKey,
        FName InExcludeMappingName,
        TArray<FCk_KeyBinding_ConflictInfo>& OutConflicts,
        ECk_KeyConflictScope InScope = ECk_KeyConflictScope::All);

    // --- Conflict Resolution ---

    /**
     * Swap two key bindings. Assigns InNewKey to InMappingName and moves
     * InMappingName's old key to whatever action currently holds InNewKey.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Swap Keys")
    static bool
    SwapKeys(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey);

    /**
     * Unbind all mappings that use the given key, then remap InMappingName to it.
     * Resolves a conflict by clearing the conflicting binding.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Unbind Conflict And Remap")
    static bool
    UnbindConflictAndRemap(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey);
};

// --------------------------------------------------------------------------------------------------------------------
