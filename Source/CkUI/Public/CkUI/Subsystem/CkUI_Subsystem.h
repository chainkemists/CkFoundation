// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkUI_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonActivatableWidget;
class UCk_UI_Layout_UE;
class UCk_Watermark_UserWidget_UE;
class UCk_UI_LayerConfigAsset_UE;
class APlayerController;
class SWidget;
class UUserWidget;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE(FCk_Delegate_Subsystem_OnPlayerAdded);
DECLARE_MULTICAST_DELEGATE(FCk_Delegate_Subsystem_OnPlayerRemoved);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-player UI subsystem that manages the UI layout and provides
 * centralized access to UI operations.
 *
 * Layout creation is explicit - call CreatePlayerLayout with a config asset
 * to create the layout, and DestroyPlayerLayout to tear it down.
 *
 * Input mode is managed through CommonUI's input routing system. Each layer
 * provides its desired input configuration via GetDesiredInputConfig().
 * For temporary input blocking during async operations, use
 * UCk_Utils_UI_UE::SuspendInput/ResumeInput.
 */
UCLASS(DisplayName = "CkSubsystem_UI")
class CKUI_API UCk_UI_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

protected:
    virtual void PlayerControllerChanged(APlayerController* InNewPlayerController) override;

    // ----------------------------------------------------------------------------------------------------------------
    // Player Management
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Create Player UI Layout")
    void CreatePlayerLayout(UCk_UI_LayerConfigAsset_UE* InConfigAsset);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Destroy Player UI Layout")
    void DestroyPlayerLayout();

    // ----------------------------------------------------------------------------------------------------------------
    // Layout Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Try Get Current Layout")
    UCk_UI_Layout_UE* TryGet_CurrentLayout() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Has Layout")
    bool Has_Layout() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget To Layer",
        meta = (Categories = "UI.Layer", DeterminesOutputType = "InWidgetClass", DynamicOutputParam = "ReturnValue"))
    UCommonActivatableWidget* PushWidgetToLayer(
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget To Layer (Soft)",
        meta = (Categories = "UI.Layer"))
    void PushWidgetToLayer_Soft(
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady);

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
    // Context Registry
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Context To Widget")
    void InjectContextToWidget(UUserWidget* InWidget, const FCk_UI_Context& InContext);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Clear Context From Widget")
    void ClearContextFromWidget(UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context For Widget")
    FCk_UI_Context Get_ContextForWidget(const UUserWidget* InWidget) const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Has Valid Context For Widget")
    bool Has_ValidContextForWidget(const UUserWidget* InWidget) const;

    // ----------------------------------------------------------------------------------------------------------------
    // Input Mode Query
    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Returns the effective input mode based on active layers. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Effective Input Mode")
    ECk_UI_InputMode Get_EffectiveInputMode() const;

    // ----------------------------------------------------------------------------------------------------------------
    // Watermark
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Set Watermark Display Policy")
    void Request_UpdateWatermarkDisplayPolicy(ECk_Watermark_DisplayPolicy InDisplayPolicy) const;

    // ----------------------------------------------------------------------------------------------------------------
    // Screen Fade
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Add Screen Fade")
    void Request_AddScreenFadeWidget(
        const FCk_ScreenFade_Params& InFadeParams,
        const APlayerController* InOwningPlayer = nullptr,
        int32 InZOrder = 100);

    // ----------------------------------------------------------------------------------------------------------------
    // Events
    // ----------------------------------------------------------------------------------------------------------------

public:
    FCk_Delegate_Subsystem_OnPlayerAdded OnPlayerAdded;
    FCk_Delegate_Subsystem_OnPlayerRemoved OnPlayerRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI")
    FCk_MulticastDelegate_UI_OnWidgetPushed OnWidgetPushed;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI")
    FCk_MulticastDelegate_UI_OnWidgetPopped OnWidgetPopped;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI")
    FCk_MulticastDelegate_UI_OnLayerCleared OnLayerCleared;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI")
    FCk_MulticastDelegate_UI_OnInputModeChanged OnInputModeChanged;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Layout Management
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoCreateLayout(UCk_UI_LayerConfigAsset_UE* InConfigAsset) -> void;
    auto DoDestroyLayout() -> void;
    auto DoBindLayoutEvents() -> void;
    auto DoUnbindLayoutEvents() -> void;
    auto DoLoadAndPushStartingWidgets(UCk_UI_LayerConfigAsset_UE* InConfigAsset) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Watermark
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoCreateWatermarkWidget(APlayerController* InPlayerController) -> void;
    auto DoDestroyWatermarkWidget() -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Screen Fade
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoRemoveScreenFadeWidget(const APlayerController* InOwningPlayer, int32 InControllerID) -> void;
    auto DoRemoveScreenFadeWidget(int32 InControllerID) -> void;
    auto DoGet_PlayerControllerID(const APlayerController* InPlayerController) const -> int32;
    auto DoGet_PlayerControllerFromID(int32 InControllerID) const -> APlayerController*;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Context Registry
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoRegisterWidget(UUserWidget* InWidget, const FCk_UI_Context& InContext) -> void;
    auto DoUnregisterWidget(UUserWidget* InWidget) -> void;
    auto DoInjectContextRecursive(UUserWidget* InWidget, const FCk_UI_Context& InContext) -> void;
    auto DoClearContextRecursive(UUserWidget* InWidget) -> void;

    UFUNCTION()
    void HandleWidgetDestroyed(UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Event Handlers
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto HandleLayoutWidgetPushed(FGameplayTag InLayerTag, UCommonActivatableWidget* InWidget) const -> void;
    auto HandleLayoutWidgetPopped(FGameplayTag InLayerTag, UCommonActivatableWidget* InWidget) const -> void;
    auto HandleLayoutLayerCleared(FGameplayTag InLayerTag) const -> void;
    auto HandleLayoutInputModeChanged(ECk_UI_InputMode InNewMode) const -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    FCk_Registry _Registry;

    FCk_Handle _SubsystemEntity;

    UPROPERTY(Transient)
    TObjectPtr<UCk_UI_Layout_UE> _Layout;

    UPROPERTY(Transient)
    TObjectPtr<UCk_Watermark_UserWidget_UE> _WatermarkWidget;

    TMap<int32, TWeakPtr<SWidget>> _FadeWidgetsForID;

    UPROPERTY(Transient)
    TMap<TObjectPtr<UUserWidget>, FCk_UI_Context> _WidgetContexts;

    static constexpr int32 InvalidPlayerControllerID = INT_MIN;
};

// --------------------------------------------------------------------------------------------------------------------