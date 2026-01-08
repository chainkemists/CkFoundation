// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUI/Types/CkUI_Types.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"

#include <CommonActivatableWidget.h>

#include "CkUserWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Base class for CkFoundation user widgets.
 *
 * Provides:
 * - Context injection (Entity, Actor, Object) - context is stored in the subsystem registry
 * - Lifecycle hooks for layer push/pop events
 * - Transition protection to prevent destruction during layer transitions
 *
 * Widgets inheriting from this class automatically participate in the
 * CkFoundation UI system's context management. Context is stored centrally
 * in the UI subsystem, so widgets don't need to manage their own context storage.
 *
 * To access context, use the getter methods (Get_ContextEntity, Get_ContextActor, etc.)
 * which query the subsystem's registry.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_UserWidget_UE
    : public UCommonActivatableWidget
    , public ICk_UI_ContextReceiver
    , public ICk_UI_LifecycleObserver
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UserWidget_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Context System - ICk_UI_ContextReceiver Implementation
    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Called when context is injected into this widget.
     * Override to respond to context changes.
     * Note: The context is stored in the subsystem, not in this widget.
     */
    virtual void OnContextInjected_Implementation(const FCk_UI_Context& InContext) override;

    /**
     * Called when context is cleared from this widget.
     * Override to respond to context being cleared.
     */
    virtual void OnContextCleared_Implementation() override;

    virtual bool Get_ShouldInheritContextFromParent_Implementation() const override { return _InheritContextFromParent; }

    // ----------------------------------------------------------------------------------------------------------------
    // Lifecycle Observer - ICk_UI_LifecycleObserver Implementation
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual void OnPrePushToLayer_Implementation(FGameplayTag InLayerTag) override;
    virtual void OnPostPushToLayer_Implementation(FGameplayTag InLayerTag) override;
    virtual void OnPrePopFromLayer_Implementation(FGameplayTag InLayerTag) override;
    virtual void OnPostPopFromLayer_Implementation(FGameplayTag InLayerTag) override;

    // ----------------------------------------------------------------------------------------------------------------
    // Context Accessors
    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Gets the entity from the current context.
     * Queries the subsystem's context registry.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    FCk_Handle Get_ContextEntity() const;

    /**
     * Gets the actor from the current context.
     * Queries the subsystem's context registry.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    AActor* Get_ContextActor() const;

    /**
     * Gets the payload object from the current context.
     * Queries the subsystem's context registry.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    UObject* Get_ContextPayload() const;


    // ----------------------------------------------------------------------------------------------------------------
    // Layer Tag Accessor
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Lifecycle")
    FGameplayTag Get_CurrentLayerTag() const { return _CurrentLayerTag; }

    // ----------------------------------------------------------------------------------------------------------------
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
#if WITH_EDITOR
    virtual const FText GetPaletteCategory() override;
#endif

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    /** If true, this widget will receive context from its parent widget. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Context")
    bool _InheritContextFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Context")
    bool _ClearContextWhenDeactivated = false;

    /** If true, prevents NativeDestruct during layer transitions. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Lifecycle")
    bool _DoNotDestroyDuringTransitions = false;

    /** The layer tag this widget is currently in. Set by lifecycle hooks. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Ck|UI|Lifecycle")
    FGameplayTag _CurrentLayerTag;

public:
    CK_PROPERTY_GET(_InheritContextFromParent);
    CK_PROPERTY_GET(_ClearContextWhenDeactivated);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------