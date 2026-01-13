// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/UserWidget/CkActivatableUserWidget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Context/CkUI_Context_Utils.h"
#include "CkUI/UserWidget/CkUserWidget.h"

#if WITH_EDITOR
#include <Editor/WidgetCompilerLog.h>
#endif

#define LOCTEXT_NAMESPACE "CkFoundation"

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_ContextReceiver Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableUserWidget_UE::
    OnContextInjected_Implementation(
        const FCk_UI_Context& InContext)
    -> void
{
    // Override in derived classes to respond to context changes
}

auto
    UCk_ActivatableUserWidget_UE::
    OnContextCleared_Implementation()
    -> void
{
    // Override in derived classes to respond to context being cleared
}

auto
    UCk_ActivatableUserWidget_UE::
    Get_ShouldInheritContextFromParent_Implementation() const
    -> bool
{
    return _InheritContextFromParent;
}

// --------------------------------------------------------------------------------------------------------------------
// ICk_UI_LayerParticipant Implementation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableUserWidget_UE::
    OnPrePushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = InLayerTag;
}

auto
    UCk_ActivatableUserWidget_UE::
    OnPostPushToLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    // Override in derived classes
}

auto
    UCk_ActivatableUserWidget_UE::
    OnPrePopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    // Override in derived classes
}

auto
    UCk_ActivatableUserWidget_UE::
    OnPostPopFromLayer_Implementation(
        FGameplayTag InLayerTag)
    -> void
{
    _CurrentLayerTag = FGameplayTag::EmptyTag;
}

// --------------------------------------------------------------------------------------------------------------------
// Context Accessors
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActivatableUserWidget_UE::
    Get_ContextEntity() const
    -> FCk_Handle
{
    return UCk_Utils_UI_Context_UE::Get_ContextEntity(this);
}

auto
    UCk_ActivatableUserWidget_UE::
    Get_ContextActor() const
    -> AActor*
{
    return UCk_Utils_UI_Context_UE::Get_ContextActor(this);
}

auto
    UCk_ActivatableUserWidget_UE::
    Get_ContextPayload() const
    -> UObject*
{
    return UCk_Utils_UI_Context_UE::Get_ContextPayload(this);
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ActivatableUserWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_ActivatableUserWidget_UE::
    NativeDestruct()
    -> void
{
    if (NOT _DoNotDestroyDuringTransitions)
    {
        Super::NativeDestruct();
    }
}

auto
    UCk_ActivatableUserWidget_UE::
    NativeOnDeactivated()
    -> void
{
    if (_ClearContextWhenDeactivated)
    {
        UCk_Utils_UI_Context_UE::ClearContext(this);
    }

    Super::NativeOnDeactivated();
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ActivatableUserWidget_UE::
    ValidateCompiledWidgetTree(
        const UWidgetTree& BlueprintWidgetTree,
        class IWidgetCompilerLog& CompileLog) const
    -> void
{
    Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

    if (NOT GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UCk_ActivatableUserWidget_UE, BP_GetDesiredFocusTarget)))
    {
        if (GetParentNativeClass(GetClass()) == StaticClass())
        {
            CompileLog.Warning(LOCTEXT("ValidateGetDesiredFocusTarget_Warning", "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen."));
        }
        else
        {
            //TODO - Note for now, because we can't guarantee it isn't implemented in a native subclass of this one.
            CompileLog.Note(LOCTEXT("ValidateGetDesiredFocusTarget_Note", "GetDesiredFocusTarget wasn't implemented, you're going to have trouble using gamepads on this screen.  If it was implemented in the native base class you can ignore this message."));
        }
    }
}
#endif

#undef LOCTEXT_NAMESPACE