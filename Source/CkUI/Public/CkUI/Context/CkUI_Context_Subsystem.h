// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkUI_Context_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UUserWidget;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-player subsystem that manages UI context injection.
 *
 * Context is stored in a central registry keyed by widget pointer.
 * Widgets implementing ICk_UI_ContextReceiver are notified on injection/clearing.
 * Context propagates recursively to child widgets that opt-in to inheritance.
 */
UCLASS(DisplayName = "CkSubsystem_UI_Context")
class CKUI_API UCk_UI_Context_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Context_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    virtual auto Deinitialize() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Context Operations
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
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoRegisterWidget(UUserWidget* InWidget, const FCk_UI_Context& InContext) -> void;
    auto DoUnregisterWidget(UUserWidget* InWidget) -> void;
    auto DoInjectContextRecursive(UUserWidget* InWidget, const FCk_UI_Context& InContext) -> void;
    auto DoClearContextRecursive(UUserWidget* InWidget) -> void;

    UFUNCTION()
    void HandleWidgetDestroyed(UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<UUserWidget>, FCk_UI_Context> _WidgetContexts;
};

// --------------------------------------------------------------------------------------------------------------------