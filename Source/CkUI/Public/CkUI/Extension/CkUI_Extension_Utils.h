// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Extension/CkUI_Extension_Types.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUI_Extension_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class ULocalPlayer;
class UUserWidget;
class UCk_UI_Extension_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Utility functions for UI extension operations.
 */
UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_UI_Extension_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_Extension_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Get Extension Subsystem",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_Extension_Subsystem_UE* Get_ExtensionSubsystem(const APlayerController* InPlayerController);

    static UCk_UI_Extension_Subsystem_UE* Get_ExtensionSubsystem(const ULocalPlayer* InLocalPlayer);

    // ----------------------------------------------------------------------------------------------------------------
    // Extension Registration
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Register Extension",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Slot"))
    static FCk_UI_ExtensionHandle RegisterExtension(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority = -1);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", DisplayName = "[Ck][UI] Unregister Extension")
    static void UnregisterExtension(
        UPARAM(ref) FCk_UI_ExtensionHandle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension")
    static bool IsValid_Extension(
        UPARAM(ref) FCk_UI_ExtensionHandle& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", DisplayName = "[Ck][UI] Unregister Extension Point")
    static void UnregisterExtensionPoint(
        UPARAM(ref) FCk_UI_ExtensionPointHandle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension")
    static bool IsValid_ExtensionPoint
    (UPARAM(ref) FCk_UI_ExtensionPointHandle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------