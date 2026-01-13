// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Context/CkUI_Context_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/CkUI_Utils.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"

#include <Blueprint/UserWidget.h>

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Lifecycle
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Context_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
}

auto
    UCk_UI_Context_Subsystem_UE::
    Deinitialize()
    -> void
{
    _WidgetContexts.Empty();
    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------
// Context Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Context_Subsystem_UE::
    InjectContextToWidget(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (InWidget->Implements<UCk_UI_ContextReceiver>())
    {
        const auto* StoredContext = _WidgetContexts.Find(InWidget);

        if ([[maybe_unused]] const auto IsContextDifferent = (ck::Is_NOT_Valid(StoredContext, ck::IsValid_Policy_NullptrOnly{})) || (*StoredContext != InContext))
        {
            DoRegisterWidget(InWidget, InContext);
            ICk_UI_ContextReceiver::Execute_OnContextInjected(InWidget, InContext);
        }
    }

    DoInjectContextRecursive(InWidget, InContext);
}

auto
    UCk_UI_Context_Subsystem_UE::
    ClearContextFromWidget(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (InWidget->Implements<UCk_UI_ContextReceiver>())
    {
        if (const auto* StoredContext = _WidgetContexts.Find(InWidget);
            ck::IsValid(StoredContext, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*StoredContext))
        {
            DoUnregisterWidget(InWidget);
            ICk_UI_ContextReceiver::Execute_OnContextCleared(InWidget);
        }
        else
        {
            DoUnregisterWidget(InWidget);
        }
    }

    DoClearContextRecursive(InWidget);
}

auto
    UCk_UI_Context_Subsystem_UE::
    Get_ContextForWidget(
        const UUserWidget* InWidget) const
    -> FCk_UI_Context
{
    if (ck::Is_NOT_Valid(InWidget))
    { return {}; }

    const auto* FoundContext = _WidgetContexts.Find(InWidget);

    if (FoundContext == nullptr)
    { return {}; }

    return *FoundContext;
}

auto
    UCk_UI_Context_Subsystem_UE::
    Has_ValidContextForWidget(
        const UUserWidget* InWidget) const
    -> bool
{
    return Get_ContextForWidget(InWidget).IsValid();
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Context_Subsystem_UE::
    DoRegisterWidget(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    const auto IsNewEntry = NOT _WidgetContexts.Contains(InWidget);

    _WidgetContexts.Add(InWidget, InContext);

    if (IsNewEntry)
    { InWidget->OnNativeDestruct.AddUObject(this, &ThisClass::HandleWidgetDestroyed); }
}

auto
    UCk_UI_Context_Subsystem_UE::
    DoUnregisterWidget(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    _WidgetContexts.Remove(InWidget);
    InWidget->OnNativeDestruct.RemoveAll(this);
}

auto
    UCk_UI_Context_Subsystem_UE::
    DoInjectContextRecursive(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(InWidget, [this, &InContext](UWidget* InTreeWidget) -> bool
    {
        if (NOT InTreeWidget->Implements<UCk_UI_ContextReceiver>())
        { return false; }

        if (const auto ShouldInherit = ICk_UI_ContextReceiver::Execute_Get_ShouldInheritContextFromParent(InTreeWidget);
            NOT ShouldInherit)
        { return true; }

        if (auto* UserWidget = Cast<UUserWidget>(InTreeWidget);
            ck::IsValid(UserWidget))
        {
            const auto* StoredContext = _WidgetContexts.Find(UserWidget);

            if ([[maybe_unused]] const auto IsContextDifferent = (ck::Is_NOT_Valid(StoredContext, ck::IsValid_Policy_NullptrOnly{})) || (*StoredContext != InContext))
            {
                DoRegisterWidget(UserWidget, InContext);
                ICk_UI_ContextReceiver::Execute_OnContextInjected(UserWidget, InContext);
            }
        }
        else
        {
            ICk_UI_ContextReceiver::Execute_OnContextInjected(InTreeWidget, InContext);
        }

        return false;
    });
}

auto
    UCk_UI_Context_Subsystem_UE::
    DoClearContextRecursive(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(InWidget, [this](UWidget* InTreeWidget) -> bool
    {
        if (NOT InTreeWidget->Implements<UCk_UI_ContextReceiver>())
        { return false; }

        if (const auto ShouldInherit = ICk_UI_ContextReceiver::Execute_Get_ShouldInheritContextFromParent(InTreeWidget);
            NOT ShouldInherit)
        { return true; }

        if (auto* UserWidget = Cast<UUserWidget>(InTreeWidget);
            ck::IsValid(UserWidget))
        {
            if (const auto* StoredContext = _WidgetContexts.Find(UserWidget);
                ck::IsValid(StoredContext, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*StoredContext))
            {
                DoUnregisterWidget(UserWidget);
                ICk_UI_ContextReceiver::Execute_OnContextCleared(UserWidget);
            }
            else
            { DoUnregisterWidget(UserWidget); }
        }
        else
        {
            ICk_UI_ContextReceiver::Execute_OnContextCleared(InTreeWidget);
        }

        return false;
    });
}

auto
    UCk_UI_Context_Subsystem_UE::
    HandleWidgetDestroyed(
        UUserWidget* InWidget)
    -> void
{
    _WidgetContexts.Remove(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------