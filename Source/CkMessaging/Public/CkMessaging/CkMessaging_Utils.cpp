#include "CkMessaging_Utils.h"

#include "CkMessaging/CkMessaging_Fragment.h"
#include "CkMessaging/CkMessaging_Log.h"

#include <Blueprint/BlueprintExceptionInfo.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkMessaging"

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Messaging_UE::execINTERNAL__Broadcast)
{
    P_GET_STRUCT_REF(FCk_Handle, Handle);
    P_GET_STRUCT(FGameplayTag, MessageName);


    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    const void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_SetInvalidValueWarning", "Failed to resolve the Value for Broadcast"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }
    else
    {
        P_NATIVE_BEGIN;
        FInstancedStruct InstancedStruct;
        InstancedStruct.InitializeAs(ValueProp->Struct, static_cast<const uint8*>(ValuePtr));
        Broadcast(Handle, MessageName, InstancedStruct);
        P_NATIVE_END;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Messaging_UE::
    Broadcast(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        FInstancedStruct InPayload)
    -> void
{
    RecordOfMessengers_Utils::AddIfMissing(InHandle);
    const auto MessengerEntity = DoGet_MessengerEntity(InHandle, InMessageName);

    ck::UUtils_Signal_Messaging::Broadcast(MessengerEntity, ck::MakePayload(InHandle, InMessageName, InPayload));
}

auto
    UCk_Utils_Messaging_UE::
    BindTo_OnBroadcast(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate)
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
