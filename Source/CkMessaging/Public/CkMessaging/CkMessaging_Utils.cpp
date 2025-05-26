#include "CkMessaging_Utils.h"

#include "CkMessaging/CkMessaging_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkMessaging"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Messaging_UE::
    Broadcast(
        FCk_Handle& InHandle,
        FName InMessageName,
        FInstancedStruct InPayload)
    -> void
{
    RecordOfMessengers_Utils::AddIfMissing(InHandle);
    const auto MessengerEntity = DoGetOrCreate_MessengerEntity(InHandle, InMessageName);

    ck::UUtils_Signal_Messaging::Broadcast(MessengerEntity, ck::MakePayload(InHandle, InPayload));
}

auto
    UCk_Utils_Messaging_UE::
    BindTo_OnBroadcast(
        FCk_Handle& InHandle,
        FName InMessageName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate)
    -> void
{
    RecordOfMessengers_Utils::AddIfMissing(InHandle);
    const auto MessengerEntity = DoGetOrCreate_MessengerEntity(InHandle, InMessageName);

    CK_SIGNAL_BIND(ck::UUtils_Signal_Messaging, MessengerEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
}

auto
    UCk_Utils_Messaging_UE::
    UnbindFrom_OnBroadcast(
        FCk_Handle& InHandle,
        FName InMessageName,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate)
    -> void
{
    const auto& MessengerEntity = RecordOfMessengers_Utils::Get_ValidEntry_ByName(InHandle, InMessageName);

    if (ck::Is_NOT_Valid(MessengerEntity))
    { return; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Messaging, MessengerEntity, InDelegate);
}

auto
    UCk_Utils_Messaging_UE::
    DoGetOrCreate_MessengerEntity(
        FCk_Handle& InHandle,
        FName InMessageName)
    -> FCk_Handle
{
    auto MessengerEntity = RecordOfMessengers_Utils::Get_ValidEntry_ByName(InHandle, InMessageName);

    if (ck::Is_NOT_Valid(MessengerEntity))
    {
        MessengerEntity  = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
        RecordOfMessengers_Utils::Request_Connect(InHandle, MessengerEntity);
    }

    return MessengerEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MessageListener_UE::
    Stop(
        FCk_Handle_MessageListener& InMessageListener)
    -> void
{
    auto MessageListener = InMessageListener.Get_MessageListener();

    if (ck::Is_NOT_Valid(MessageListener))
    { return; }

    UCk_Utils_Messaging_UE::UnbindFrom_OnBroadcast(
        MessageListener,
        InMessageListener.Get_MessageName(),
        InMessageListener.Get_MessageDelegate());
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
