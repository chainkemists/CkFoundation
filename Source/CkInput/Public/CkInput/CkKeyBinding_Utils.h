#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <GameFramework/PlayerController.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkKeyBinding_Utils.generated.h"

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
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Get Input User Settings")
    static UEnhancedInputUserSettings*
    Get_InputUserSettings(
        APlayerController* InPlayerController);

    /**
     * Get all player-mappable key bindings from the active key profile.
     * Only Input Actions with a UPlayerMappableKeySettings are included.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Get All Remappable Keys")
    static TArray<FPlayerKeyMapping>
    Get_AllRemappableKeys(
        APlayerController* InPlayerController);

    // --- Remapping ---

    /**
     * Rebind a key mapping to a new key.
     * @param InPlayerController  The player controller
     * @param InMappingName       Unique mapping name (e.g. "IA_Jump") from UPlayerMappableKeySettings
     * @param InSlot              Which slot to rebind (First, Second, etc.)
     * @param InNewKey            The new FKey to bind
     * @return                    True if the rebind succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Remap Key")
    static bool
    RemapKey(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey);

    // --- Reset ---

    /** Reset a single mapping row back to its default key(s). */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Reset Mapping To Default")
    static void
    ResetMappingToDefault(
        APlayerController* InPlayerController,
        FName InMappingName);

    /** Reset ALL key mappings on the active profile to their defaults. */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Reset All Keys To Defaults")
    static void
    ResetAllToDefaults(
        APlayerController* InPlayerController);

    // --- Persistence ---

    /** Persist the current key bindings to disk asynchronously. */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Save Key Bindings")
    static void
    SaveKeyBindings(
        APlayerController* InPlayerController);

    // --- Conflict Detection ---

    /**
     * Check if a key is already bound to another action.
     * @param InPlayerController   The player controller
     * @param InNewKey             The key to check for conflicts
     * @param InExcludeMappingName The mapping being rebound (excluded from results)
     * @param OutConflictingNames  Display names of conflicting actions (for UI)
     * @return                     True if there are conflicts
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Get Has Key Conflicts")
    static bool
    Get_HasKeyConflicts(
        APlayerController* InPlayerController,
        FKey InNewKey,
        FName InExcludeMappingName,
        TArray<FText>& OutConflictingNames);

    // --- Conflict Resolution ---

    /**
     * Swap two key bindings. Assigns InNewKey to InMappingName and moves
     * InMappingName's old key to whatever action currently holds InNewKey.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Swap Keys")
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
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck] Unbind Conflict And Remap")
    static bool
    UnbindConflictAndRemap(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey);
};

// --------------------------------------------------------------------------------------------------------------------
