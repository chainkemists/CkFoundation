#pragma once

#include <CkEcs/Handle/CkHandle.h>

#include <CkEcs/CkNet_Utils.h>
#include <CkEcs/TimeSync/CkNetTimeSync_Fragment_Data.h>

#include "CkNetTimeSync_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_NetTimeSync_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_NetTimeSync_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Add Replicated Feature (INTERNAL USE ONLY)")
    static void
    Add_NetTimeSync_Rep(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Add Feature")
    static void
    Add(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InHandle);

public:
    static auto
    Request_NewTimeSync(
        FCk_Handle InHandle,
        FCk_Request_NetTimeSync_NewSync InRequest) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Get Round Trip Time")
    static FCk_Time
    Get_RoundTripTime(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Get Latency")
    static FCk_Time
    Get_Latency(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Get Player Round Trip Time")
    static FCk_Time
    Get_PlayerRoundTripTime(
        APlayerController* InPlayerController,
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[Ck][NetworkTimeSync] Get Player Latency")
    static FCk_Time
    Get_PlayerLatency(
        APlayerController* InPlayerController,
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
