// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkEcs/ContextReceiver/CkContextReceiver.h"
#include "CkUI/Layout/CkUI_LayerStack.h"

#include <CommonActivatableWidget.h>

#include "CkUI_LayerStack_ContextProvider.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_ContextInjectionMode : uint8
{
    Never,
    OnlyIfMissing,
    Always
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_ContextInjectionMode);

// --------------------------------------------------------------------------------------------------------------------

/**
 * LayerStack variant that participates in context propagation.
 *
 * When this layer stack receives context (via its _ContextReceiver property),
 * it conditionally injects that context into widgets as they are pushed.
 *
 * Injection is controlled by ECk_UI_ContextInjectionMode:
 * - Never: widgets must receive context explicitly
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

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context")
    void
    Set_InjectionMode(ECk_UI_ContextInjectionMode InMode);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    ECk_UI_ContextInjectionMode
    Get_InjectionMode() const;

    // ----------------------------------------------------------------------------------------------------------------
    // UCk_UI_LayerStack_UE Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual auto OnPostWidgetPush(UCommonActivatableWidget* InWidget) -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

protected:
    virtual auto ShouldInjectContextToWidget(UCommonActivatableWidget* InWidget) const -> bool;

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
    ECk_UI_ContextInjectionMode _InjectionMode = ECk_UI_ContextInjectionMode::OnlyIfMissing;

public:
    CK_PROPERTY_GET(_ContextReceiver);
};

// --------------------------------------------------------------------------------------------------------------------
