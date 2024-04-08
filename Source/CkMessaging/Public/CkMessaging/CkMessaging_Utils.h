#pragma once

#include "CkMessaging/CkMessaging_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkMessaging/CkMessaging_Fragment.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkMessaging_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKMESSAGING_API UCk_Utils_Messaging_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Messaging_UE);

private:
    struct RecordOfMessengers_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMessengers> {};

public:
    UFUNCTION(BlueprintCallable,
              CustomThunk,
              Category = "Ck|Utils|Messaging",
              DisplayName="[Ck][Messaging] Broadcast",
              //meta=(CustomStructureParam = "InValue", BlueprintInternalUseOnly = true))
              meta=(CustomStructureParam = "InValue"))
    static void
    INTERNAL__Broadcast(
        FCk_Handle InHandle,
        FGameplayTag InMessageName,
        const int32& InValue);
    DECLARE_FUNCTION(execINTERNAL__Broadcast);

    static auto
    Broadcast(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        const FInstancedStruct& InPayload) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Messaging",
              DisplayName="[Ck][Messaging] BindTo Broadcast")
    static void
    BindTo_OnBroadcast(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Messaging",
              DisplayName="[Ck][Messaging] Unbind From Broadcast")
    static void
    UnbindFrom_OnBroadcast(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InMessageName,
        const FCk_Delegate_Messaging_OnBroadcast& InDelegate);

private:
    static auto
    DoGet_MessengerEntity(
        FCk_Handle& InHandle,
        FGameplayTag InMessageName) -> FCk_Handle;
};

// --------------------------------------------------------------------------------------------------------------------
