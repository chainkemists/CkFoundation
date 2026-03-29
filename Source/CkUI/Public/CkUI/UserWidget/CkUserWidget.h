// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"

#include <CommonUserWidget.h>

#include "CkUserWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Base class for non-activatable CkFoundation user widgets.
 *
 * Provides:
 * - Context injection (Entity, Actor, Object) - context stored in subsystem registry
 * - Automatic context inheritance from parent widgets
 *
 * For activatable widgets (push/pop from layers), use UCk_ActivatableUserWidget_UE.
 *
 * Context is stored centrally in the UI context subsystem, so widgets don't need
 * to manage their own context storage. Use the getter methods to access context.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_UserWidget_UE
    : public UCommonUserWidget
    , public ICk_UI_ContextReceiver
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UserWidget_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // ICk_UI_ContextReceiver Implementation
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto OnContextInjected_Implementation(const FCk_UI_Context& InContext) -> void override;
    auto OnContextCleared_Implementation() -> void override;
    auto Get_ShouldInheritContextFromParent_Implementation() const -> bool override;

    // ----------------------------------------------------------------------------------------------------------------
    // Context Accessors
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure)
    FCk_Handle Get_ContextEntity() const;

    UFUNCTION(BlueprintPure)
    AActor* Get_ContextActor() const;

    UFUNCTION(BlueprintPure)
    UObject* Get_ContextPayload() const;

    // ----------------------------------------------------------------------------------------------------------------
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

    auto NativeDestruct() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool _InheritContextFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool _DoNotDestroyDuringTransitions = false;

public:
    CK_PROPERTY_GET(_InheritContextFromParent);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------