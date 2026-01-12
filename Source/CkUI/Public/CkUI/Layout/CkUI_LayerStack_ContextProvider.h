// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkUI/Interfaces/CkUI_Interfaces.h"
#include "CkUI/Layout/CkUI_LayerStack.h"
#include "CkUI/Types/CkUI_Types.h"

#include <CommonActivatableWidget.h>

#include "CkUI_LayerStack_ContextProvider.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Defines when the layer should inject its stored context into pushed widgets.
 */
UENUM(BlueprintType)
enum class ECk_UI_ContextInjectionMode : uint8
{
    /** Never auto-inject - widgets must receive context explicitly */
    Never,

    /** Only inject if the widget has no valid context already (default) */
    OnlyIfMissing,

    /** Always inject, overwriting any existing context */
    Always
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_ContextInjectionMode);

// --------------------------------------------------------------------------------------------------------------------

/**
 * LayerStack variant that participates in context propagation.
 *
 * When a Layout receives context, it cascades to child LayerStacks via the
 * standard ICk_UI_ContextReceiver mechanism. This class stores that context
 * and conditionally applies it to widgets as they are pushed to the layer.
 *
 * Usage:
 *   1. Use this class (or a BP subclass) for layers that should provide context
 *   2. Inject context to the owning Layout
 *   3. Widgets pushed to this layer will inherit context based on InjectionMode
 */
UCLASS(DisplayName = "CkUI_LayerStack_ContextProvider")
class CKUI_API UCk_UI_LayerStack_ContextProvider_UE
    : public UCk_UI_LayerStack_UE
    , public ICk_UI_ContextReceiver
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_LayerStack_ContextProvider_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // ICk_UI_ContextReceiver
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto OnContextInjected_Implementation(const FCk_UI_Context& InContext) -> void override;
    auto OnContextCleared_Implementation() -> void override;
    auto Get_ShouldInheritContextFromParent_Implementation() const -> bool override;

    // ----------------------------------------------------------------------------------------------------------------
    // Context Provider API
    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Gets the currently stored context for this layer. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Layer Context")
    FCk_UI_Context Get_LayerContext() const;

    /** Returns true if this layer has a valid stored context. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Has Valid Layer Context")
    bool Has_ValidLayerContext() const;

    /** Sets the context injection mode for this layer. */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Set Injection Mode")
    void Set_InjectionMode(ECk_UI_ContextInjectionMode InMode);

    /** Gets the current context injection mode. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Injection Mode")
    ECk_UI_ContextInjectionMode Get_InjectionMode() const;

    /** Sets whether this layer should inherit context from its parent layout. */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Set Should Inherit From Parent")
    void Set_ShouldInheritFromParent(bool InShouldInherit);

    // ----------------------------------------------------------------------------------------------------------------
    // UCk_UI_LayerStack_UE Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual auto OnPostWidgetPush(UCommonActivatableWidget* InWidget) -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

protected:
    /**
     * Called after a widget is pushed to determine if context should be injected.
     * Override in subclasses for custom injection logic.
     */
    virtual auto ShouldInjectContextToWidget(UCommonActivatableWidget* InWidget) const -> bool;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Context",
        meta = (DisplayName = "Injection Mode"))
    ECk_UI_ContextInjectionMode _InjectionMode = ECk_UI_ContextInjectionMode::OnlyIfMissing;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Context",
        meta = (DisplayName = "Inherit Context From Parent"))
    bool _ShouldInheritFromParent = true;

private:
    UPROPERTY(Transient)
    FCk_UI_Context _StoredContext;
};

// --------------------------------------------------------------------------------------------------------------------