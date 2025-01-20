#include "CkWidgetLayerHandler_Utils.h"

#include "CkUI/Subsystem/CkUI_Subsystem.h"
#include "CkUI/WidgetLayerHandler/CkWidgetLayerHandler_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_WidgetLayerHandler_UE, FCk_Handle_WidgetLayerHandler, ck::FFragment_WidgetLayerHandler_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Get_WidgetLayerHandler(
        APlayerController* InPlayerController)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController),
        TEXT("Invalid PlayerController supplied to Get_WidgetLayerHandler"))
    { return {}; }

    const auto& LocalPlayer = InPlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return {}; }

    const auto& UISubsystem = LocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

    if (ck::Is_NOT_Valid(UISubsystem))
    { return {}; }

    return UISubsystem->Get_WidgetLayerHandler();
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_WidgetLayerHandler_ParamsData& InParams)
    -> FCk_Handle_WidgetLayerHandler
{
    InHandle.Add<ck::FFragment_WidgetLayerHandler_Params>(InParams);
    return Cast(InHandle);
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_PushToLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_PushToLayer& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnPushToLayer::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer(), InRequest.Get_WidgetClass()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_PushToLayer_Instanced(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_PushToLayer_Instanced& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnPushToLayer_Instanced::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer(), InRequest.Get_WidgetInstance()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_PopFromLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_PopFromLayer& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnPopFromLayer::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_ClearLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_ClearLayer& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnClearLayer::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_AddWidgetToLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_AddWidgetToLayerNamedSlot& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnAddWidgetToLayerNamedSlot::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer(), InRequest.Get_WidgetClass(), InRequest.Get_NamedSlotName()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_AddWidgetInstanceToLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_AddWidgetInstanceToLayerNamedSlot& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer(), InRequest.Get_WidgetInstance(), InRequest.Get_NamedSlotName()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    Request_RemoveWidgetFromLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Request_WidgetLayerHandler_RemoveWidgetFromLayerNamedSlot& InRequest)
    -> FCk_Handle_WidgetLayerHandler
{
    ck::UUtils_Signal_WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Layer(), InRequest.Get_NamedSlotName()));
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnPushToLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnPushToLayer, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnPushToLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnPushToLayer, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnPushToLayer_Instanced(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnPushToLayer_Instanced, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnPushToLayer_Instanced(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnPushToLayer_Instanced& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnPushToLayer_Instanced, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnPopFromLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnPopFromLayer& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnPopFromLayer, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnPopFromLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnPopFromLayer& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnPopFromLayer, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnClearLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnClearLayer& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnClearLayer, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnClearLayer(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnClearLayer& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnClearLayer, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnAddWidgetToLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnAddWidgetToLayerNamedSlot& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnAddWidgetToLayerNamedSlot, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnAddWidgetToLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnAddWidgetToLayerNamedSlot& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnAddWidgetToLayerNamedSlot, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnAddWidgetInstanceToLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnAddWidgetInstanceToLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnAddWidgetInstanceToLayerNamedSlot, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    BindTo_OnRemoveWidgetFromLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_WidgetLayerHandler_UE::
    UnbindFrom_OnRemoveWidgetFromLayerNamedSlot(
        FCk_Handle_WidgetLayerHandler& InHandle,
        const FCk_Delegate_WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot& InDelegate)
    -> FCk_Handle_WidgetLayerHandler
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_WidgetLayerHandler_OnRemoveWidgetFromLayerNamedSlot, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------