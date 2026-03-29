// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUI_Layout_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class ULocalPlayer;
class UCommonActivatableWidget;
class UCk_UI_Layout_Subsystem_UE;
class UCk_UI_PrimaryGameLayout_UE;
class UCk_UI_LayerStack_UE;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Utility functions for UI layout and layer operations.
 */
UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_UI_Layout_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_Layout_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Layout Subsystem",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_Layout_Subsystem_UE* Get_LayoutSubsystem(const APlayerController* InPlayerController);

    static UCk_UI_Layout_Subsystem_UE* Get_LayoutSubsystem(const ULocalPlayer* InLocalPlayer);

    // ----------------------------------------------------------------------------------------------------------------
    // Layout Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Layout",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_PrimaryGameLayout_UE* Get_Layout(const APlayerController* InPlayerController);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static UCk_UI_LayerStack_UE* Get_Layer(const APlayerController* InPlayerController, FGameplayTag InLayerTag);

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Push Widget To Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer",
            DeterminesOutputType = "InWidgetClass", DynamicOutputParam = "ReturnValue"))
    static UCommonActivatableWidget* PushWidgetToLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Push Widget To Layer (Soft)",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static void PushWidgetToLayer_Soft(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Push Widget To Layer (Instance)",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static UCommonActivatableWidget* PushWidgetToLayer_Instance(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Pop Widget From Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static UCommonActivatableWidget* PopWidgetFromLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Clear Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static void ClearLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Remove Widget",
        meta = (DefaultToSelf = "InPlayerController"))
    static bool RemoveWidget(
        const APlayerController* InPlayerController,
        UCommonActivatableWidget* InWidget);
     
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Remove Widget (Self)",
        meta = (DefaultToSelf = "InWidget"))
    static bool RemoveWidgetSelf(UCommonActivatableWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Mode
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Effective Input Mode",
        meta = (DefaultToSelf = "InPlayerController"))
    static ECk_UI_InputMode Get_EffectiveInputMode(const APlayerController* InPlayerController);
};

// --------------------------------------------------------------------------------------------------------------------