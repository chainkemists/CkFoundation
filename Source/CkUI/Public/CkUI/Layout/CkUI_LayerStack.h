// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/CustomWidgets/WidgetStack/CkStack_Widget.h"
#include "CkUI/Types/CkUI_Types.h"

#include <GameplayTagContainer.h>

#include "CkUI_LayerStack.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * A stack-based container for activatable widgets within a UI layer.
 *
 * Extends the base stack widget with layer-specific functionality:
 * - Layer tag identification
 * - Priority for input mode resolution
 * - Default input mode when this layer has widgets
 * - Lifecycle notifications for push/pop events (ICk_UI_LayerParticipant)
 *
 * Events inherited from base class:
 * - OnWidgetPushed / OnWidgetPopped (from UCk_WidgetStack_UE)
 * - OnTransitioningChanged (from UCommonActivatableWidgetContainerBase)
 */
UCLASS(DisplayName = "CkUI_LayerStack")
class CKUI_API UCk_UI_LayerStack_UE : public UCk_WidgetStack_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_LayerStack_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Configuration
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto SetLayerTag(FGameplayTag InTag) -> void;
    auto SetPriority(int32 InPriority) -> void;
    auto SetDefaultInputMode(ECk_UI_InputMode InMode) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // UCk_Stack_UserWidget_UE Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    auto OnPreWidgetPush(UCommonActivatableWidget* InWidget) -> void override;
    auto OnPostWidgetPush(UCommonActivatableWidget* InWidget) -> void override;
    auto OnPreWidgetPop(UCommonActivatableWidget* InWidget) -> void override;
    auto OnPostWidgetPop(UCommonActivatableWidget* InWidget) -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Lifecycle Notifications
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoNotifyParticipantPrePush(UCommonActivatableWidget* InWidget) const -> void;
    auto DoNotifyParticipantPostPush(UCommonActivatableWidget* InWidget) const -> void;
    auto DoNotifyParticipantPrePop(UCommonActivatableWidget* InWidget) const -> void;
    auto DoNotifyParticipantPostPop(UCommonActivatableWidget* InWidget) const -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(EditAnywhere, Category = "Layer", meta = (AllowPrivateAccess = true, Categories = "UI.Layer"))
    FGameplayTag _LayerTag;

    UPROPERTY(EditAnywhere, Category = "Layer", meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

    UPROPERTY(EditAnywhere, Category = "Layer", meta = (AllowPrivateAccess = true))
    ECk_UI_InputMode _DefaultInputMode = ECk_UI_InputMode::GameOnly;

public:
    CK_PROPERTY_GET(_LayerTag);
    CK_PROPERTY_GET(_Priority);
    CK_PROPERTY_GET(_DefaultInputMode);
};

// --------------------------------------------------------------------------------------------------------------------