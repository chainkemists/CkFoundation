#pragma once

#include <CkEcs/Handle/CkHandle.h>

#include <CkNet/CkNet_Utils.h>
#include <CkNet/TimeSync/CkNetTimeSync_Fragment_Data.h>

#include "CkNetTimeSync_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_NetTimeSync_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_NetTimeSync_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Add Replicated Feature (INTERNAL USE ONLY)")
    static void
    Add_NetTimeSync_Rep(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Add Feature")
    static void
    Add(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Ensure Has Feature")
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
              DisplayName = "[NetworkTimeSync] Get Round Trip Time")
    static FCk_Time
    Get_RoundTripTime(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Get Latency")
    static FCk_Time
    Get_Latency(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Get Player Round Trip Time")
    static FCk_Time
    Get_PlayerRoundTripTime(
        APlayerController* InPlayerController,
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|NetTimeSync",
              DisplayName = "[NetworkTimeSync] Get Player Latency")
    static FCk_Time
    Get_PlayerLatency(
        APlayerController* InPlayerController,
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
