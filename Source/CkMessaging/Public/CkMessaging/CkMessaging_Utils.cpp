#include "CkMessaging_Utils.h"

#include "CkCore/Validation/CkUntracedStructSafety.h"
#include "CkMessaging/CkMessaging_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkMessaging"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Messaging_UE::
    Broadcast(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        FInstancedStruct InPayload)
    -> void
{
    if (InPayload.IsValid())
    {
        const auto* ScriptStruct = InPayload.GetScriptStruct();
        const auto HasScriptStruct = ScriptStruct != nullptr;
        CK_ENSURE_IF_NOT(HasScriptStruct,
            TEXT("Message [{}] rejected an InstancedStruct payload without a reflected struct type"), InMessageName)
        { return; }

        const auto Safety = ck::Analyze_UntracedStructSafety(ScriptStruct);
        const auto IsPayloadSafe = Safety.IsGcIndependent();
        CK_ENSURE_IF_NOT(IsPayloadSafe,
            TEXT("Message [{}] rejected unsafe InstancedStruct payload [{}]; [{}]: {}"),
            InMessageName,
            ScriptStruct->GetName(),
            Safety.FailurePath,
            Safety.FailureReason)
        { return; }
    }

    RecordOfMessengers_Utils::AddIfMissing(InHandle);
    const auto MessengerEntity = DoGet_MessengerEntity(InHandle, InMessageName);

    ck::UUtils_Signal_Messaging::Broadcast(MessengerEntity, ck::MakePayload(InHandle, InMessageName, InPayload));
}

auto
    UCk_Utils_Messaging_UE::
    BindTo_OnBroadcast(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> void
{
    RecordOfMessengers_Utils::AddIfMissing(InHandle);
    const auto MessengerEntity = DoGet_MessengerEntity(InHandle, InMessageName);

    CK_SIGNAL_BIND(ck::UUtils_Signal_Messaging, MessengerEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
}

auto
    UCk_Utils_Messaging_UE::
    UnbindFrom_OnBroadcast(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate)
    -> void
{
    const auto& MessengerEntity = RecordOfMessengers_Utils::Get_ValidEntry_ByTag(InHandle, InMessageName);

    if (ck::Is_NOT_Valid(MessengerEntity))
    { return; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Messaging, MessengerEntity, InDelegate);
}

auto
    UCk_Utils_Messaging_UE::
    DoGet_MessengerEntity(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName)
    -> FCk_Handle
{
    auto MessengerEntity = RecordOfMessengers_Utils::Get_ValidEntry_ByTag(InHandle, InMessageName);

    if (ck::Is_NOT_Valid(MessengerEntity))
    {
        MessengerEntity  = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
        UCk_Utils_GameplayLabel_UE::Add(MessengerEntity, InMessageName);
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
