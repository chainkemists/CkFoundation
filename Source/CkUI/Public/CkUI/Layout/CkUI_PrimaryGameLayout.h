// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/ContextReceiver/CkContextReceiver.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Blueprint/UserWidget.h>
#include <GameplayTagContainer.h>

#include "CkUI_PrimaryGameLayout.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UI_LayerWidget_UE;
class UCk_UI_LayerStack_UE;
class UCk_UI_LayoutConfigAsset_UE;
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
 * - When no layers have widgets, the default input mode from config is used
 *
 * Input is automatically suspended during layer transitions.
 */
UCLASS(DisplayName = "CkUI_PrimaryGameLayout")
class CKUI_API UCk_UI_PrimaryGameLayout_UE : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_PrimaryGameLayout_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Initialization
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto InitializeFromConfig(UCk_UI_LayoutConfigAsset_UE* InConfigAsset) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        meta = (Categories = "UI.Layer"))
    UCk_UI_LayerStack_UE* Get_Layer(FGameplayTag InLayerTag) const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    TArray<UCk_UI_LayerStack_UE*> Get_AllLayers() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        meta = (Categories = "UI.Layer"))
    bool HasLayer(FGameplayTag InLayerTag) const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    FGameplayTag Get_ActiveLayerTag() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PushWidgetToLayer(
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PushWidgetInstanceToLayer(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        meta = (Categories = "UI.Layer"))
    UCommonActivatableWidget* PopWidgetFromLayer(FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        meta = (Categories = "UI.Layer"))
    void ClearLayer(FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI")
    bool RemoveWidget(UCommonActivatableWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Mode
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    ECk_UI_InputMode Get_EffectiveInputMode() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    ECk_UI_InputMode Get_DefaultInputMode() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Transition State
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    bool IsAnyLayerTransitioning() const;

    // ----------------------------------------------------------------------------------------------------------------
    // UCommonActivatableWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto GetDesiredInputConfig() const -> TOptional<FUIInputConfig> override;

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
    // Context
    // ----------------------------------------------------------------------------------------------------------------

private:
    UFUNCTION()
    void
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity);

    // ----------------------------------------------------------------------------------------------------------------
    // UUserWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    auto NativeConstruct() -> void override;
    auto NativeDestruct() -> void override;
    auto RebuildWidget() -> TSharedRef<SWidget> override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Layer Management
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoCreateRootOverlay() -> void;
    auto DoCreateLayers(const UCk_UI_LayoutConfigAsset_UE* InConfigAsset) -> void;
    auto DoRegisterLayer(UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto DoDestroyLayers() -> void;
    auto DoAddLayerToOverlay(UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto DoBindLayerEvents(UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto DoUnbindLayerEvents(UCk_UI_LayerWidget_UE* InWrapper) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Layer Activation
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoUpdateActiveLayer() -> void;
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
    auto HandleLayerAllWidgetsCleared(UCk_UI_LayerWidget_UE* InWrapper) const -> void;
    auto HandleLayerTransitionStateChanged(bool InIsTransitioning, UCk_UI_LayerWidget_UE* InWrapper) -> void;
    auto HandleLayerHasWidgetsChanged(bool InHasWidgets, UCk_UI_LayerWidget_UE* InWrapper) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Context")
    FCk_Handle_ContextReceiver _ContextReceiver;

private:
    UPROPERTY(Transient)
    TObjectPtr<UOverlay> _RootOverlay;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UCk_UI_LayerStack_UE>> _Layers;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_UI_LayerWidget_UE>> _LayerWrappers;

    UPROPERTY(Transient)
    TObjectPtr<UCk_UI_LayerWidget_UE> _ActiveLayerWrapper;

    ECk_UI_InputMode _DefaultInputMode = ECk_UI_InputMode::GameOnly;
    EMouseCaptureMode _DefaultMouseCaptureMode = EMouseCaptureMode::CapturePermanently;

    TOptional<ECk_UI_InputMode> _CachedInputMode;
    TArray<FCk_Handle_InputSuspension> _TransitionSuspensionHandles;
};

// --------------------------------------------------------------------------------------------------------------------