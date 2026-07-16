#pragma once

#include "CkActorRelay_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkActorRelay_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PendingActorRelay"))
class CKACTORRELAY_API UCk_Utils_PendingActorRelay_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PendingActorRelay_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Promise/Future OnAcquired",
              meta = (Keywords = "bind, wait, when, finish"))
    static void
    Promise_OnAcquired(
        UPARAM(ref) FCk_Handle_PendingActorRelay& InPending,
        const FCk_Delegate_ActorRelay_Acquired& InDelegate);

    static void
    Promise_OnAcquired(
        FCk_Handle_PendingActorRelay& InPending,
        TFunction<void(FCk_ActorRelay_ChannelResult)> InCallback);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Get Is Valid (Pending Actor Relay)",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid(
        const FCk_Handle_PendingActorRelay& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKACTORRELAY_API UCk_Utils_ActorRelay_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorRelay_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Channel",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle_PendingActorRelay
    Request_AcquireChannel(
        const UObject* InWorldContextObject,
        UPARAM(meta = (Categories = "ActorRelay")) FGameplayTag InGroupTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Acquire Channel For Player",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle_PendingActorRelay
    Request_AcquireChannel_ForPlayer(
        const UObject* InWorldContextObject,
        UPARAM(meta = (Categories = "ActorRelay")) FGameplayTag InGroupTag,
        APlayerState* InPlayerState);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Get Channel Entity Count")
    static int32
    Get_ChannelEntityCount(
        const FCk_ActorRelay_ChannelResult& InChannelResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorRelay",
              DisplayName = "[Ck][ActorRelay] Is Channel Result Valid")
    static bool
    Get_IsChannelResultValid(
        const FCk_ActorRelay_ChannelResult& InChannelResult);
};

// --------------------------------------------------------------------------------------------------------------------
