#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

#include "CkNet/TimeSync/CkNetTimeSync_Fragment_Params.h"

#include "CkNetTimeSync_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_NetTimeSync_UE;

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FTag_TimeSync_SyncedAtleastOnce {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNET_API FFragment_TimeSync
    {
    public:
        CK_GENERATED_BODY(FFragment_TimeSync);

        friend class FCk_Processor_TimeSync_HandleRequests;

    private:
        FCk_Time _RoundTripTime = FCk_Time::Zero;
        TMap<TObjectPtr<APlayerController>, FCk_Time> _PlayerRoundTripTimes;

    public:
        CK_PROPERTY_GET(_RoundTripTime);
        CK_PROPERTY_GET(_PlayerRoundTripTimes);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNET_API FFragment_TimeSync_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_TimeSync);

        friend class FCk_Processor_TimeSync_HandleRequests;
        friend class UCk_Utils_NetTimeSync_UE;

    public:
        using TimeSyncRequestType = FCk_Request_NetTimeSync_NewSync;
        using TimeSyncRequestList = TArray<TimeSyncRequestType>;

    private:
        TimeSyncRequestList _TimeSyncRequests;

    public:
        CK_PROPERTY_GET(_TimeSyncRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    class FCk_Processor_TimeSync_HandleRequests;
    class FCk_Processor_TimeSync_OnNetworkClockSynchronized;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKNET_API UCk_Fragment_TimeSync_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_TimeSync_Rep);

public:
    friend class ck::FCk_Processor_TimeSync_OnNetworkClockSynchronized;

public:
    UFUNCTION(Server, Unreliable)
    void
    Broadcast_TimeSync(
        APlayerController* InPlayerController,
        FCk_Time InRoundTripTime);

protected:
    virtual auto
    OnLink() -> void override;

private:
    auto
    DoBroadcast_TimeSync(
        FCk_Time InRoundTripTime) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
