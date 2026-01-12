// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkUI_Layout_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonActivatableWidget;
class UCk_UI_PrimaryGameLayout_UE;
class UCk_UI_LayoutConfigAsset_UE;
class UCk_UI_LayerStack_UE;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE(FCk_Delegate_LayoutSubsystem_OnLayoutCreated);
DECLARE_MULTICAST_DELEGATE(FCk_Delegate_LayoutSubsystem_OnLayoutDestroyed);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-player subsystem that manages the Primary Game Layout.
 *
 * Responsibilities:
 * - Creating and destroying the layout from config assets
 * - Providing layer access and widget operations
 * - Async widget loading with input suspension
 * - Broadcasting layout events
 *
 * Layout creation is explicit - call CreateLayout with a config asset.
 */
UCLASS(DisplayName = "CkSubsystem_UI_Layout")
class CKUI_API UCk_UI_Layout_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Layout_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    virtual auto Deinitialize() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Layout Management
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Create Layout")
    void CreateLayout(UCk_UI_LayoutConfigAsset_UE* InConfigAsset);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Destroy Layout")
    void DestroyLayout();

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Get Layout")
    UCk_UI_PrimaryGameLayout_UE* Get_Layout() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Has Layout")
    bool Has_Layout() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Get Layer",
        meta = (Categories = "UI.Layer"))
    UCk_UI_LayerStack_UE* Get_Layer(FGameplayTag InLayerTag) const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Has Layer",
        meta = (Categories = "UI.Layer"))
    bool HasLayer(FGameplayTag InLayerTag) const;

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Push Widget To Layer",
        meta = (Categories = "UI.Layer", DeterminesOutputType = "InWidgetClass", DynamicOutputParam = "ReturnValue"))
    UCommonActivatableWidget* PushWidgetToLayer(
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Push Widget To Layer (Soft)",
        meta = (Categories = "UI.Layer"))
    void PushWidgetToLayer_Soft(
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Push Widget Instance To Layer",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PushWidgetInstanceToLayer(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Pop Widget From Layer",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PopWidgetFromLayer(FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Clear Layer",
        meta = (Categories = "UI.Layer"))
    void ClearLayer(FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Remove Widget")
    bool RemoveWidget(UCommonActivatableWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Mode Query
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Layout",
        DisplayName = "[Ck][UI] Get Effective Input Mode")
    ECk_UI_InputMode Get_EffectiveInputMode() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Events
    // ----------------------------------------------------------------------------------------------------------------

public:
    FCk_Delegate_LayoutSubsystem_OnLayoutCreated OnLayoutCreated;
    FCk_Delegate_LayoutSubsystem_OnLayoutDestroyed OnLayoutDestroyed;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|Layout")
    FCk_MulticastDelegate_UI_OnWidgetPushed OnWidgetPushed;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|Layout")
    FCk_MulticastDelegate_UI_OnWidgetPopped OnWidgetPopped;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|Layout")
    FCk_MulticastDelegate_UI_OnLayerCleared OnLayerCleared;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|Layout")
    FCk_MulticastDelegate_UI_OnInputModeChanged OnInputModeChanged;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoCreateLayout( UCk_UI_LayoutConfigAsset_UE* InConfigAsset) -> void;
    auto DoDestroyLayout() -> void;
    auto DoBindLayoutEvents() -> void;
    auto DoUnbindLayoutEvents() -> void;
    auto DoLoadAndPushStartingWidgets(UCk_UI_LayoutConfigAsset_UE* InConfigAsset) -> void;

    auto HandleLayoutWidgetPushed(FGameplayTag InLayerTag, UCommonActivatableWidget* InWidget) const -> void;
    auto HandleLayoutWidgetPopped(FGameplayTag InLayerTag, UCommonActivatableWidget* InWidget) const -> void;
    auto HandleLayoutLayerCleared(FGameplayTag InLayerTag) const -> void;
    auto HandleLayoutInputModeChanged(ECk_UI_InputMode InNewMode) const -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_UI_PrimaryGameLayout_UE> _Layout;
};

// --------------------------------------------------------------------------------------------------------------------