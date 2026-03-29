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
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Extension Subsystem",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_Extension_Subsystem_UE* Get_ExtensionSubsystem(const APlayerController* InPlayerController);

    static UCk_UI_Extension_Subsystem_UE* Get_ExtensionSubsystem(const ULocalPlayer* InLocalPlayer);

    // ----------------------------------------------------------------------------------------------------------------
    // Extension Registration
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Register Extension",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.ExtensionPoint"))
    static FCk_UI_ExtensionHandle RegisterExtension(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority = -1);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Register Extension (Instance)",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.ExtensionPoint"))
    static FCk_UI_ExtensionHandle RegisterExtension_Instance(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag,
        UUserWidget* InWidgetInstance,
        int32 InPriority = -1);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Unregister Extension")
    static void UnregisterExtension(
        UPARAM(ref) FCk_UI_ExtensionHandle& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Try Unregister Extension (By Widget)",
        meta = (DefaultToSelf = "InWidget"))
    static bool TryUnregisterExtensionByWidget(
        UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Is Valid (Extension)")
    static bool IsValid_Extension(
        const FCk_UI_ExtensionHandle& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Unregister Extension Point")
    static void UnregisterExtensionPoint(
        UPARAM(ref) FCk_UI_ExtensionPointHandle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension", BlueprintCosmetic,
        DisplayName = "[Ck] Get Is Valid (ExtensionPoint)")
    static bool IsValid_ExtensionPoint(
        const FCk_UI_ExtensionPointHandle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------