// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/ContextReceiver/CkContextReceiver_Utils.h"
#include "CkUI/CkUI_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Context
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UserWidget_UE::
    Get_ContextEntity() const
    -> FCk_Handle
{
    return _ContextReceiver.Get_ContextEntity();
}

auto
    UCk_UserWidget_UE::
    OnValidContextInjected_Implementation(
        const FCk_Handle& InContextEntity)
    -> void
{
}

auto
    UCk_UserWidget_UE::
    OnContextCleared_Implementation()
    -> void
{
}

// ----

auto
    UCk_UserWidget_UE::
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity)
    -> void
{
    OnValidContextInjected(InContextEntity);
    UCk_Utils_UI_UE::PropagateContextToChildWidgets(GetRootWidget(), InContextEntity);
}

auto
    UCk_UserWidget_UE::
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
    UCk_UserWidget_UE::
    NativeConstruct()
    -> void
{
    Super::NativeConstruct();

    auto InjectedDelegate = FCk_Delegate_ContextReceiver_OnContextInjected{};
    InjectedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCk_UserWidget_UE, HandleContextInjected));
    UCk_Utils_ContextReceiver_UE::BindTo_OnContextInjected(_ContextReceiver, InjectedDelegate);

    auto ClearedDelegate = FCk_Delegate_ContextReceiver_OnContextCleared{};
    ClearedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCk_UserWidget_UE, HandleContextCleared));
    UCk_Utils_ContextReceiver_UE::BindTo_OnContextCleared(_ContextReceiver, ClearedDelegate);
}

#if WITH_EDITOR
auto
    UCk_UserWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_UserWidget_UE::
    NativeDestruct()
    -> void
{
    UCk_Utils_ContextReceiver_UE::Request_UnbindAll(_ContextReceiver, this);

    if (NOT _DoNotDestroyDuringTransitions)
    {
        Super::NativeDestruct();
    }
}

// --------------------------------------------------------------------------------------------------------------------
