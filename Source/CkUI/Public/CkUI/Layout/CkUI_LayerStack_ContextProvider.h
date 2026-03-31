// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkEcs/ContextReceiver/CkContextReceiver.h"
#include "CkUI/Layout/CkUI_LayerStack.h"

#include <CommonActivatableWidget.h>

#include "CkUI_LayerStack_ContextProvider.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * LayerStack variant that participates in context propagation.
 *
 * When this layer stack receives context (via its _ContextReceiver property),
 * it conditionally injects that context into widgets as they are pushed.
 *
 * Injection is controlled by ECk_ContextInjectionMode:
 * - OnlyIfMissing: inject only if the pushed widget has no valid context
 * - Always: overwrite any existing context on the pushed widget
 *
 * Non-ContextProvider layer stacks (plain UCk_UI_LayerStack_UE) are unaffected
 * by context propagation from the owning layout.
 */
UCLASS(DisplayName = "CkUI_LayerStack_ContextProvider")
class CKUI_API UCk_UI_LayerStack_ContextProvider_UE
    : public UCk_UI_LayerStack_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_LayerStack_ContextProvider_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Provider API
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    bool
    Has_ValidLayerContext() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    FCk_Handle
    Get_LayerContextEntity() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    ECk_ContextInjectionMode
    Get_InjectionMode() const;

    // ----------------------------------------------------------------------------------------------------------------
    // UCk_UI_LayerStack_UE Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual auto OnPostWidgetPush(UCommonActivatableWidget* InWidget) -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    auto OnWidgetRebuilt() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

private:
    UFUNCTION()
    void
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity);

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Context")
    FCk_Handle_ContextReceiver _ContextReceiver;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Context",
              meta = (DisplayName = "Injection Mode"))
    ECk_ContextInjectionMode _InjectionMode = ECk_ContextInjectionMode::OnlyIfMissing;

public:
    CK_PROPERTY_GET(_ContextReceiver);
};

// --------------------------------------------------------------------------------------------------------------------
