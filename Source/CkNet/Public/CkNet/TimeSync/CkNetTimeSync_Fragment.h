#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkNet/TimeSync/CkNetTimeSync_Fragment_Params.h"

#include "CkNetTimeSync_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_NetTimeSync_UE;

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FTag_NetTimeSync_SyncedAtleastOnce {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNET_API FFragment_NetTimeSync
    {
    public:
        CK_GENERATED_BODY(FFragment_NetTimeSync);

        friend class FProcessor_NetTimeSync_HandleRequests;

    private:
        FCk_Time _RoundTripTime;
        TMap<TObjectPtr<APlayerController>, FCk_Time> _PlayerRoundTripTimes;

    public:
        CK_PROPERTY_GET(_RoundTripTime);
        CK_PROPERTY_GET(_PlayerRoundTripTimes);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNET_API FFragment_NetTimeSync_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_NetTimeSync);

        friend class FProcessor_NetTimeSync_HandleRequests;
        friend class UCk_Utils_NetTimeSync_UE;

    public:
        using TimeSyncRequestType = FCk_Request_NetTimeSync_NewSync;
        using TimeSyncRequestList = TArray<TimeSyncRequestType>;

    private:
        TimeSyncRequestList _NetTimeSyncRequests;

    public:
        CK_PROPERTY_GET(_NetTimeSyncRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_NetTimeSync_HandleRequests;
    class FProcessor_NetTimeSync_OnNetworkClockSynchronized;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKNET_API UCk_Fragment_NetTimeSync_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_NetTimeSync_Rep);

public:
    friend class ck::FProcessor_NetTimeSync_OnNetworkClockSynchronized;

public:
    UFUNCTION(Server, Unreliable)
    void
    Broadcast_NetTimeSync(
        APlayerController* InPlayerController,
        FCk_Time InRoundTripTime);

    UFUNCTION(NetMulticast, Unreliable)
    void
    Broadcast_TimeSync_Clients(
        APlayerController* InPlayerController,
        FCk_Time InRoundTripTime);

private:
    auto
    DoBroadcast_NetTimeSync(
        FCk_Time InRoundTripTime) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
