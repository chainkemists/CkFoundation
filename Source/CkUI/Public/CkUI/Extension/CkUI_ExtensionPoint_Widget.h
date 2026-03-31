// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/ContextReceiver/CkContextReceiver.h"
#include "CkUI/Extension/CkUI_Extension_Types.h"

#include <Components/DynamicEntryBoxBase.h>

#include "CkUI_ExtensionPoint_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class IWidgetCompilerLog;
class UUserWidget;

// --------------------------------------------------------------------------------------------------------------------

/**
 * A widget that defines an extension point slot in the UI hierarchy.
 *
 * Place this widget where you want dynamic content to be added. Extensions
 * registered with matching tags will be automatically instantiated and added
 * as children of this widget.
 */
UCLASS()
class CKUI_API UCk_UI_ExtensionPoint_Widget_UE : public UDynamicEntryBoxBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_ExtensionPoint_Widget_UE);

    UCk_UI_ExtensionPoint_Widget_UE(const FObjectInitializer& ObjectInitializer);

    // ----------------------------------------------------------------------------------------------------------------
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto ReleaseSlateResources(bool InReleaseChildren) -> void override;
    virtual auto RebuildWidget() -> TSharedRef<SWidget> override;

#if WITH_EDITOR
    virtual auto ValidateCompiledDefaults(IWidgetCompilerLog& InCompileLog) const -> void override;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extension",
        meta = (Categories = "UI.ExtensionPoint"))
    FGameplayTag _ExtensionPointTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Extension")
    ECk_UI_ExtensionPointMatch _MatchType = ECk_UI_ExtensionPointMatch::ExactMatch;

    // ----------------------------------------------------------------------------------------------------------------
    // Context
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Extension|Context")
    FCk_Handle_ContextReceiver _ContextReceiver;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Extension|Context",
              meta = (DisplayName = "Context Injection Mode"))
    ECk_ContextInjectionMode _InjectionMode = ECk_ContextInjectionMode::OnlyIfMissing;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoResetExtensionPoint() -> void;
    auto DoRegisterExtensionPoint() -> void;
    auto DoGetExtensionSubsystem() const -> UCk_UI_Extension_Subsystem_UE*;
    auto HandleExtensionAddedOrRemoved(ECk_UI_ExtensionAction InAction, const FCk_UI_ExtensionRequest& InRequest) -> void;

    auto DoTryInjectContextIntoWidget(UUserWidget* InWidget) const -> void;
    auto DoReInjectContextToAllExtensions() -> void;

    UFUNCTION()
    void
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity);

    // ----------------------------------------------------------------------------------------------------------------
    // Runtime State
    // ----------------------------------------------------------------------------------------------------------------

private:
    FCk_UI_ExtensionPointHandle _ExtensionPointHandle;

    UPROPERTY(Transient)
    TMap<FCk_UI_ExtensionHandle, TObjectPtr<UUserWidget>> _ExtensionMapping;

public:
    CK_PROPERTY_GET(_ExtensionPointTag);
    CK_PROPERTY_GET(_MatchType);
    CK_PROPERTY_GET(_ContextReceiver);
    CK_PROPERTY_GET(_InjectionMode);
};

// --------------------------------------------------------------------------------------------------------------------