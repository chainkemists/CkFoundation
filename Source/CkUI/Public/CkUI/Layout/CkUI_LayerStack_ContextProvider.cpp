// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_LayerStack_ContextProvider.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/ContextReceiver/CkContextReceiver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Context Provider API
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Has_ValidLayerContext() const
    -> bool
{
    return UCk_Utils_ContextReceiver_UE::Has_ValidContext(_ContextReceiver);
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Get_LayerContextEntity() const
    -> FCk_Handle
{
    return _ContextReceiver.Get_ContextEntity();
}

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    Get_InjectionMode() const
    -> ECk_ContextInjectionMode
{
    return _InjectionMode;
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    OnWidgetRebuilt()
    -> void
{
    Super::OnWidgetRebuilt();

    auto InjectedDelegate = FCk_Delegate_ContextReceiver_OnContextInjected{};
    InjectedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCk_UI_LayerStack_ContextProvider_UE, HandleContextInjected));
    UCk_Utils_ContextReceiver_UE::BindTo_OnContextInjected(_ContextReceiver, InjectedDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
// UCk_UI_LayerStack_UE Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    OnPostWidgetPush(
        UCommonActivatableWidget* InWidget)
    -> void
{
    Super::OnPostWidgetPush(InWidget);

    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    const auto& ContextEntity = _ContextReceiver.Get_ContextEntity();
    UCk_Utils_ContextReceiver_UE::TryInjectContextIntoObject(InWidget, ContextEntity, _InjectionMode);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerStack_ContextProvider_UE::
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity)
    -> void
{
    const auto& ExistingWidgets = GetWidgetList();

    ck::algo::ForEachIsValid(ExistingWidgets, [this](UCommonActivatableWidget* InWidget)
    {
        const auto& ContextEntity = _ContextReceiver.Get_ContextEntity();
        UCk_Utils_ContextReceiver_UE::TryInjectContextIntoObject(InWidget, ContextEntity, _InjectionMode);
    });
}

// --------------------------------------------------------------------------------------------------------------------
