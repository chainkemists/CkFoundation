#pragma once

#include "CkECS/Handle/CkHandle.h"

#include "CkMessaging/CkMessaging_Fragment.h"
#include "CkMessaging/CkMessaging_Fragment_Data.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkMessaging_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKMESSAGING_API UCk_Utils_Messaging_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Messaging_UE);

private:
    struct RecordOfMessengers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMessengers> {};

public:
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
    static void
    Broadcast(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        FInstancedStruct InPayload);

public:
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              Category = "Ck|Utils|Messaging",
              DisplayName="[Ck][Messaging] Bind To OnBroadcast")
    static void
    BindTo_OnBroadcast(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "Message")) FGameplayTag InMessageName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              Category = "Ck|Utils|Messaging",
              DisplayName="[Ck][Messaging] Unbind From OnBroadcast")
    static void
    UnbindFrom_OnBroadcast(
        UPARAM(ref) FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "Message")) FGameplayTag InMessageName,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate);

private:
    static auto
    DoGet_MessengerEntity(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName) -> FCk_Handle;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKMESSAGING_API UCk_Utils_MessageListener_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MessageListener_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Messaging",
              DisplayName = "[Ck][Messaging] Stop Listening For Messages")
    static void
    Stop(
        UPARAM(ref) FCk_Handle_MessageListener& InMessageListener);
};

// --------------------------------------------------------------------------------------------------------------------
