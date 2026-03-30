// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/UserWidget/CkActivatableWidget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/ContextReceiver/CkContextReceiver_Utils.h"
#include "CkUI/CkUI_Utils.h"

#if WITH_EDITOR
#include <Editor/WidgetCompilerLog.h>
#endif

#define LOCTEXT_NAMESPACE "CkFoundation"

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_LayerParticipant Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableWidget_UE::
    OnPrePushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = InLayerTag;
}

auto
    UCk_ActivatableWidget_UE::
    OnPostPushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
}

auto
    UCk_ActivatableWidget_UE::
    OnPrePopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
}

auto
    UCk_ActivatableWidget_UE::
    OnPostPopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = FGameplayTag::EmptyTag;
}

// --------------------------------------------------------------------------------------------------------------------
// Context
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableWidget_UE::
    Get_ContextEntity() const
    -> FCk_Handle
{
    return _ContextReceiver.Get_ContextEntity();
}

auto
    UCk_ActivatableWidget_UE::
    OnValidContextInjected_Implementation(
        const FCk_Handle& InContextEntity)
    -> void
{
}

auto
    UCk_ActivatableWidget_UE::
    OnContextCleared_Implementation()
    -> void
{
}

// ----

auto
    UCk_ActivatableWidget_UE::
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity)
    -> void
{
    OnValidContextInjected(InContextEntity);
    UCk_Utils_UI_UE::PropagateContextToChildWidgets(GetRootWidget(), InContextEntity);
}

auto
    UCk_ActivatableWidget_UE::
    HandleContextCleared(
        FCk_Handle_ContextReceiver InContextReceiver)
    -> void
{
    OnContextCleared();
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableWidget_UE::
    NativeConstruct()
    -> void
{
    Super::NativeConstruct();

    auto InjectedDelegate = FCk_Delegate_ContextReceiver_OnContextInjected{};
    InjectedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCk_ActivatableWidget_UE, HandleContextInjected));
    UCk_Utils_ContextReceiver_UE::BindTo_OnContextInjected(_ContextReceiver, InjectedDelegate);

    auto ClearedDelegate = FCk_Delegate_ContextReceiver_OnContextCleared{};
    ClearedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCk_ActivatableWidget_UE, HandleContextCleared));
    UCk_Utils_ContextReceiver_UE::BindTo_OnContextCleared(_ContextReceiver, ClearedDelegate);
}

#if WITH_EDITOR
auto
    UCk_ActivatableWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_ActivatableWidget_UE::
    NativeDestruct()
    -> void
{
    UCk_Utils_ContextReceiver_UE::Request_UnbindAll(_ContextReceiver, this);

    if (NOT _DoNotDestroyDuringTransitions)
    {
        Super::NativeDestruct();
    }
}

auto
    UCk_ActivatableWidget_UE::
    NativeOnDeactivated()
    -> void
{
    if (_ClearContextWhenDeactivated)
    {
        UCk_Utils_ContextReceiver_UE::Request_ClearContext(_ContextReceiver);
    }

    Super::NativeOnDeactivated();
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ActivatableWidget_UE::
    ValidateCompiledWidgetTree(
        const UWidgetTree& BlueprintWidgetTree,
        class IWidgetCompilerLog& CompileLog) const
    -> void
{
    Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

    if (NOT GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UCk_ActivatableWidget_UE, BP_GetDesiredFocusTarget)))
    {
        if (GetParentNativeClass(GetClass()) == StaticClass())
        {
            CompileLog.Warning(LOCTEXT("ValidateGetDesiredFocusTarget_Warning", "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen."));
        }
        else
        {
            CompileLog.Note(LOCTEXT("ValidateGetDesiredFocusTarget_Note", "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen.  If it was implemented in the native base class you can ignore this message."));
        }
    }
}
#endif

#undef LOCTEXT_NAMESPACE
