// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Blueprint/UserWidget.h>
#include <GameplayTagContainer.h>

#include "CkUI_Layout.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UI_LayerWidget_UE;
class UCk_UI_LayerStack_UE;
class UCk_UI_LayerConfigAsset_UE;
class UCommonActivatableWidget;
class UOverlay;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE_TwoParams(FCk_Delegate_Layout_OnWidgetPushed, FGameplayTag, UCommonActivatableWidget*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FCk_Delegate_Layout_OnWidgetPopped, FGameplayTag, UCommonActivatableWidget*);
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_Layout_OnLayerCleared, FGameplayTag);
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_Layout_OnInputModeChanged, ECk_UI_InputMode);
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_Layout_OnActiveLayerChanged, FGameplayTag);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Root widget that manages UI layers for a player.
 *
 * Layers are created from a configuration asset and provide stack-based
 * widget management. The layout manages layer activation to ensure only
 * one layer is activated at a time for proper CommonUI input routing.
 *
 * Activation Priority:
 * - Only the highest-priority layer with active widgets is activated
 * - When a layer gains/loses widgets, activation is re-evaluated
 * - The active layer's GetDesiredInputConfig() determines the input mode
 *
 * Input is automatically suspended during layer transitions to prevent
 * input from reaching partially-visible widgets.
 */
UCLASS(DisplayName = "CkUI_Layout")
class CKUI_API UCk_UI_Layout_UE : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Layout_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Initialization
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto InitializeFromConfig(UCk_UI_LayerConfigAsset_UE* InConfigAsset) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Layer",
        meta = (Categories = "UI.Layer"))
    UCk_UI_LayerStack_UE* Get_Layer(FGameplayTag InLayerTag) const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get All Layers")
    TArray<UCk_UI_LayerStack_UE*> Get_AllLayers() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Has Layer",
        meta = (Categories = "UI.Layer"))
    bool HasLayer(FGameplayTag InLayerTag) const;

    /** Returns the tag of the currently active layer, or an invalid tag if none. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Active Layer Tag")
    FGameplayTag Get_ActiveLayerTag() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget To Layer",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PushWidgetToLayer(
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget Instance To Layer",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PushWidgetInstanceToLayer(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Pop Widget From Layer",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PopWidgetFromLayer(FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Clear Layer",
        meta = (Categories = "UI.Layer"))
    void ClearLayer(FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Remove Widget")
    bool RemoveWidget(UCommonActivatableWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Mode
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Effective Input Mode")
    ECk_UI_InputMode Get_EffectiveInputMode() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Transition State
    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Returns true if any layer is currently transitioning. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Is Any Layer Transitioning")
    bool IsAnyLayerTransitioning() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Events
    // ----------------------------------------------------------------------------------------------------------------

public:
    FCk_Delegate_Layout_OnWidgetPushed OnWidgetPushed;
    FCk_Delegate_Layout_OnWidgetPopped OnWidgetPopped;
    FCk_Delegate_Layout_OnLayerCleared OnLayerCleared;
    FCk_Delegate_Layout_OnInputModeChanged OnInputModeChanged;
    FCk_Delegate_Layout_OnActiveLayerChanged OnActiveLayerChanged;

    // ----------------------------------------------------------------------------------------------------------------
    // UUserWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Layer Management
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoCreateRootOverlay() -> void;
    auto DoCreateLayers(const UCk_UI_LayerConfigAsset_UE* InConfigAsset) -> void;
    auto DoRegisterLayer(UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto DoDestroyLayers() -> void;
    auto DoAddLayerToOverlay(UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto DoBindLayerEvents(UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto DoUnbindLayerEvents(UCk_UI_LayerWidget_UE* InWrapper) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Layer Activation
    // ----------------------------------------------------------------------------------------------------------------

private:
    /**
     * Re-evaluates which layer should be active based on priority.
     * Activates the highest-priority layer with widgets, deactivates others.
     */
    auto DoUpdateActiveLayer() -> void;

    /**
     * Finds the highest-priority layer wrapper that currently has widgets.
     * Returns nullptr if no layers have widgets.
     */
    auto DoFindHighestPriorityLayerWithWidgets() const -> UCk_UI_LayerWidget_UE*;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Input Mode
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoUpdateInputMode() -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Transition Handling
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoHandleTransitionStateChanged(UCk_UI_LayerWidget_UE* InWrapper, bool InIsTransitioning) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Event Handlers
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto HandleLayerWidgetPushed(UCommonActivatableWidget* InWidget, UCk_UI_LayerWidget_UE* InWrapper) const -> void;
    auto HandleLayerWidgetPopped(UCommonActivatableWidget* InWidget, UCk_UI_LayerWidget_UE* InWrapper) const -> void;
    auto HandleLayerTransitionStateChanged(bool InIsTransitioning, UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto HandleLayerHasWidgetsChanged(bool InHasWidgets, UCk_UI_LayerWidget_UE* InWrapper) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(Transient)
    TObjectPtr<UOverlay> _RootOverlay;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UCk_UI_LayerStack_UE>> _Layers;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_UI_LayerWidget_UE>> _LayerWrappers;

    /** The currently activated layer wrapper. Only one layer is active at a time. */
    UPROPERTY(Transient)
    TObjectPtr<UCk_UI_LayerWidget_UE> _ActiveLayerWrapper;

    TOptional<ECk_UI_InputMode> _CachedInputMode;

    /**
     * Stack of input suspension tokens for transitioning layers.
     * Multiple layers can transition simultaneously, so we track each token separately.
     */
    TArray<FName> _TransitionSuspendTokens;
};

// --------------------------------------------------------------------------------------------------------------------