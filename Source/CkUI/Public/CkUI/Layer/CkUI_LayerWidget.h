// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkUI/Types/CkUI_Types.h"

#include <CommonActivatableWidget.h>
#include <GameplayTagContainer.h>
#include <Slate/SCommonAnimatedSwitcher.h>

#include "CkUI_LayerWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UI_LayerStack_UE;
class UCommonActivatableWidgetContainerBase;
class UOverlay;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_LayerWidget_OnWidgetPushed, UCommonActivatableWidget*);
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_LayerWidget_OnWidgetPopped, UCommonActivatableWidget*);
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_LayerWidget_OnTransitionStateChanged, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FCk_Delegate_LayerWidget_OnHasWidgetsChanged, bool);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Wrapper that enables layer stacks to participate in CommonUI input routing.
 * Provides GetDesiredInputConfig() based on the layer's ECk_UI_InputMode.
 *
 * IMPORTANT: This widget does NOT auto-activate. The owning Layout is responsible
 * for activating/deactivating layer wrappers based on priority resolution.
 * Only the highest-priority layer with active widgets should be activated at a time.
 *
 * Also forwards transition state from the underlying stack to enable
 * input suspension during widget transitions.
 */
UCLASS()
class CKUI_API UCk_UI_LayerWidget_UE : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_LayerWidget_UE);

    UCk_UI_LayerWidget_UE(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Configure(
        TSubclassOf<UCk_UI_LayerStack_UE> InStackClass,
        FGameplayTag InLayerTag,
        int32 InPriority,
        ECk_UI_InputMode InInputMode) -> void;

    auto SetTransitionSettings(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurve,
        FCk_Time InTransitionDuration) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Get_Stack() const -> UCk_UI_LayerStack_UE*;
    auto Get_LayerTag() const -> FGameplayTag;
    auto Get_Priority() const -> int32;
    auto Get_InputMode() const -> ECk_UI_InputMode;

    /** Returns true if this layer has any widgets in its stack. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    bool HasWidgets() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    bool IsTransitioning() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Events
    // ----------------------------------------------------------------------------------------------------------------

public:
    FCk_Delegate_LayerWidget_OnWidgetPushed OnWidgetPushed;
    FCk_Delegate_LayerWidget_OnWidgetPopped OnWidgetPopped;
    FCk_Delegate_LayerWidget_OnTransitionStateChanged OnTransitionStateChanged;

    /** Broadcast when the layer goes from having widgets to not having widgets (or vice versa). */
    FCk_Delegate_LayerWidget_OnHasWidgetsChanged OnHasWidgetsChanged;

    // ----------------------------------------------------------------------------------------------------------------
    // UCommonActivatableWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto GetDesiredInputConfig() const -> TOptional<FUIInputConfig> override;

    // ----------------------------------------------------------------------------------------------------------------
    // UUserWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual auto RebuildWidget() -> TSharedRef<SWidget> override;
    virtual auto NativeConstruct() -> void override;
    virtual auto NativeDestruct() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoCreateStack() -> void;
    auto DoBindStackEvents() -> void;
    auto DoUnbindStackEvents() -> void;
    auto DoUpdateHasWidgets() -> void;

    auto HandleStackWidgetPushed(UCommonActivatableWidget* InWidget) -> void;
    auto HandleStackWidgetPopped(UCommonActivatableWidget* InWidget) -> void;
    auto HandleStackTransitioningChanged(UCommonActivatableWidgetContainerBase* InContainer, bool InIsTransitioning) -> void;

    static auto MapInputModeToConfig(ECk_UI_InputMode InMode) -> FUIInputConfig;

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(Transient)
    TSubclassOf<UCk_UI_LayerStack_UE> _StackClass;

    FGameplayTag _LayerTag;
    int32 _Priority = 0;
    ECk_UI_InputMode _InputMode = ECk_UI_InputMode::GameOnly;

    TOptional<ECommonSwitcherTransition> _TransitionType;
    TOptional<ETransitionCurve> _TransitionCurve;
    TOptional<FCk_Time> _TransitionDuration;

    // ----------------------------------------------------------------------------------------------------------------
    // Runtime Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(Transient)
    TObjectPtr<UOverlay> _RootOverlay;

    UPROPERTY(Transient)
    TObjectPtr<UCk_UI_LayerStack_UE> _Stack;

    bool _HasWidgets = false;
    bool _IsTransitioning = false;
};

// --------------------------------------------------------------------------------------------------------------------