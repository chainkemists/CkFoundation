#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <GameFramework/PlayerController.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkKeyBinding_Utils.generated.h"

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

    /** The key that is currently bound to this conflicting mapping. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FKey _CurrentKey = FKey{EKeys::Invalid};

    /** Which slot the conflict is in (First, Second, etc.). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    EPlayerMappableKeySlot _Slot = EPlayerMappableKeySlot::Unspecified;

public:
    CK_PROPERTY_GET(_MappingName);
    CK_PROPERTY_GET(_DisplayName);
    CK_PROPERTY_GET(_CurrentKey);
    CK_PROPERTY_GET(_Slot);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_KeyBinding_ConflictInfo, _MappingName, _DisplayName, _CurrentKey, _Slot);
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
     * @param OutConflicts         Detailed info about each conflicting binding (mapping name, display name, key, slot)
     * @return                     True if there are conflicts
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyBinding", DisplayName = "[Ck][KeyBinding] Get Has Key Conflicts")
    static bool
    Get_HasKeyConflicts(
        APlayerController* InPlayerController,
        FKey InNewKey,
        FName InExcludeMappingName,
        TArray<FCk_KeyBinding_ConflictInfo>& OutConflicts);

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
