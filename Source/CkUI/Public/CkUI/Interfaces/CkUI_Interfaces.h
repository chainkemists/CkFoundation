// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkUI/Types/CkUI_Types.h"

#include <GameplayTagContainer.h>
#include <UObject/Interface.h>

#include "CkUI_Interfaces.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_ContextReceiver
// --------------------------------------------------------------------------------------------------------------------

/**
 * Interface for widgets that can receive context injection.
 *
 * Context is managed by the Context Subsystem's registry. Widgets implementing
 * this interface will be notified when context is injected or cleared.
 *
 * To query context, use UCk_Utils_UI_Context_UE::Get_ContextForWidget() or
 * UCk_UI_Context_Subsystem_UE::Get_ContextForWidget() rather than storing context locally.
 */
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UCk_UI_ContextReceiver : public UInterface { GENERATED_BODY() };
class CKUI_API ICk_UI_ContextReceiver
{
    GENERATED_BODY()

public:
    /**
     * Called when context is injected into this widget.
     * The widget should use this to react to the new context (e.g., update displays).
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Context")
    void OnContextInjected(const FCk_UI_Context& InContext);

    /**
     * Called when context is cleared from this widget.
     * The widget should use this to reset any context-dependent state.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Context")
    void OnContextCleared();

    /**
     * Returns whether this widget should inherit context from its parent widget.
     * Default is true - override to prevent automatic context propagation.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Context")
    bool Get_ShouldInheritContextFromParent() const;
};

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_LayerParticipant
// --------------------------------------------------------------------------------------------------------------------

/**
 * Interface for widgets that want to observe layer lifecycle events.
 * Notified when the widget is pushed to or popped from a layer stack.
 */
UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UCk_UI_LayerParticipant : public UInterface { GENERATED_BODY() };
class CKUI_API ICk_UI_LayerParticipant
{
    GENERATED_BODY()

public:
    /** Called immediately before the widget is pushed to a layer. */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Lifecycle")
    void OnPrePushToLayer(FGameplayTag InLayerTag);

    /** Called immediately after the widget is pushed to a layer. */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Lifecycle")
    void OnPostPushToLayer(FGameplayTag InLayerTag);

    /** Called immediately before the widget is popped from a layer. */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Lifecycle")
    void OnPrePopFromLayer(FGameplayTag InLayerTag);

    /** Called immediately after the widget is popped from a layer. */
    UFUNCTION(BlueprintNativeEvent, Category = "Ck|UI|Lifecycle")
    void OnPostPopFromLayer(FGameplayTag InLayerTag);
};

// --------------------------------------------------------------------------------------------------------------------