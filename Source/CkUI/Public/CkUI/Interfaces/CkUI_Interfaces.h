// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include <GameplayTagContainer.h>
#include <UObject/Interface.h>

#include "CkUI_Interfaces.generated.h"

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