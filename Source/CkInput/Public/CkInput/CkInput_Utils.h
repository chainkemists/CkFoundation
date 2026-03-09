#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <Framework/Commands/InputChord.h>
#include <GameFramework/PlayerController.h>

#include "CkInput_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UInputMappingContext;

// --------------------------------------------------------------------------------------------------------------------

/** Pairs an Input Mapping Context with a priority for batch Add operations. */
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_MappingContextWithPriority
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MappingContextWithPriority);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UInputMappingContext> _MappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

public:
    CK_PROPERTY(_MappingContext);
    CK_PROPERTY(_Priority);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MappingContextWithPriority, _MappingContext);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKINPUT_API UCk_Utils_Input_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Input_UE);

public:

    // --- Input Key Queries ---

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input", DisplayName = "[Ck] Was Input Chord Just Pressed")
    static bool
    WasInputChordJustPressed(
        APlayerController* InPlayerController,
        const FInputChord& InInputChord);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input", DisplayName = "[Ck] Was Input Key Just Pressed")
    static bool
    WasInputKeyJustPressed(
        APlayerController* InPlayerController,
        const FKey& InInputKey);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Input", DisplayName = "[Ck] Was Input Key Just Pressed (With Custom Modifier)")
    static bool
    WasInputKeyJustPressed_WithCustomModifier(
        APlayerController* InPlayerController,
        const FKey& InInputKey,
        const FKey& InCustomModiferKey);

    // --- Mapping Context Management ---

    /**
     * Add one or more Input Mapping Contexts to the player's Enhanced Input subsystem.
     * @param InPlayerController  The player controller
     * @param InContexts          Array of mapping contexts with their priorities
     * @param InClearPrevious     If true, removes all existing mapping contexts before adding
     * @return                    True if the operation succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|MappingContext", DisplayName = "[Ck][Input] Add Input Mapping Contexts", meta = (AutoCreateRefTerm = "InContexts"))
    static bool
    AddMappingContexts(
        APlayerController* InPlayerController,
        const TArray<FCk_MappingContextWithPriority>& InContexts,
        bool InClearPrevious = false);

    /**
     * Remove one or more Input Mapping Contexts from the player's Enhanced Input subsystem.
     * @param InPlayerController  The player controller
     * @param InContexts          Array of mapping contexts to remove
     * @return                    True if the operation succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|MappingContext", DisplayName = "[Ck][Input] Remove Input Mapping Contexts", meta = (AutoCreateRefTerm = "InContexts"))
    static bool
    RemoveMappingContexts(
        APlayerController* InPlayerController,
        const TArray<TSoftObjectPtr<UInputMappingContext>>& InContexts);

    /**
     * Swap one Input Mapping Context for another.
     * Removes InPreviousContext and adds InNewContext in a single operation.
     * @param InPlayerController      The player controller
     * @param InPreviousContext        The mapping context to remove
     * @param InNewContext             The mapping context to add
     * @param InPriority               Priority for the new context (ignored if InUsePreviousPriority is true)
     * @param InUsePreviousPriority    If true, the new context inherits the removed context's priority
     * @return                         True if the operation succeeded
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|MappingContext", DisplayName = "[Ck][Input] Swap Input Mapping Contexts")
    static bool
    SwapMappingContexts(
        APlayerController* InPlayerController,
        TSoftObjectPtr<UInputMappingContext> InPreviousContext,
        TSoftObjectPtr<UInputMappingContext> InNewContext,
        int32 InPriority = 0,
        bool InUsePreviousPriority = false);
};

// --------------------------------------------------------------------------------------------------------------------
