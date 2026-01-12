// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"

#include <CommonActivatableWidget.h>

#include "CkActivatableUserWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Base class for activatable CkFoundation user widgets.
 *
 * Provides:
 * - Context injection (Entity, Actor, Object) - context stored in subsystem registry
 * - Lifecycle hooks for layer push/pop events (ICk_UI_LayerParticipant)
 * - Transition protection to prevent destruction during layer transitions
 *
 * Use this class for widgets that will be pushed/popped from UI layers.
 * For non-activatable widgets, use UCk_UserWidget_UE.
 *
 * Context is stored centrally in the UI context subsystem, so widgets don't need
 * to manage their own context storage. Use the getter methods to access context.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_ActivatableUserWidget_UE
    : public UCommonActivatableWidget
    , public ICk_UI_ContextReceiver
    , public ICk_UI_LayerParticipant
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActivatableUserWidget_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // ICk_UI_ContextReceiver Implementation
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto OnContextInjected_Implementation(const FCk_UI_Context& InContext) -> void override;
    auto OnContextCleared_Implementation() -> void override;
    auto Get_ShouldInheritContextFromParent_Implementation() const -> bool override;

    // ----------------------------------------------------------------------------------------------------------------
    // ICk_UI_LayerParticipant Implementation
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto OnPrePushToLayer_Implementation(FGameplayTag InLayerTag) -> void override;
    auto OnPostPushToLayer_Implementation(FGameplayTag InLayerTag) -> void override;
    auto OnPrePopFromLayer_Implementation(FGameplayTag InLayerTag) -> void override;
    auto OnPostPopFromLayer_Implementation(FGameplayTag InLayerTag) -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Context Accessors
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    FCk_Handle Get_ContextEntity() const;

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context")
    AActor* Get_ContextActor() const;

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
    auto GetPaletteCategory() -> const FText override;
#endif

    auto NativeDestruct() -> void override;
    auto NativeOnDeactivated() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Context")
    bool _InheritContextFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Context")
    bool _ClearContextWhenDeactivated = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Lifecycle")
    bool _DoNotDestroyDuringTransitions = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Ck|UI|Lifecycle")
    FGameplayTag _CurrentLayerTag;

public:
    CK_PROPERTY_GET(_InheritContextFromParent);
    CK_PROPERTY_GET(_ClearContextWhenDeactivated);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------