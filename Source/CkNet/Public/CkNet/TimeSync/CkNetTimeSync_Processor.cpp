#include "CkNetTimeSync_Processor.h"

#include "NetworkTimeSubsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkNet/TimeSync/CkNetTimeSync_Settings.h"

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

auto
    FProcessor_NetTimeSync_OnNetworkClockSynchronized::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const TObjectPtr<UCk_Fragment_NetTimeSync_Rep>& InTimeSync_Rep)
    -> void
{
    if (_DelegateHandle.IsValid())
    { return; }

    _NetworkTimeSubsystem = UNetworkTimeSubsystem::Get(InTimeSync_Rep.Get());

    CK_ENSURE_IF_NOT(ck::IsValid(_NetworkTimeSubsystem),
        TEXT("Could not get a valid NetworkTimeSubsystem from [{}] on Entity [{}]"), InTimeSync_Rep, InHandle)
    { return; }

    _DelegateHandle = _NetworkTimeSubsystem->OnNetworkClockSynchronized_Cpp.AddRaw(
        this, &FProcessor_NetTimeSync_OnNetworkClockSynchronized::OnNetworkClockSynchronized);
}

FProcessor_NetTimeSync_OnNetworkClockSynchronized::
    ~FProcessor_NetTimeSync_OnNetworkClockSynchronized()
{
    if (NOT _DelegateHandle.IsValid())
    { return; }

    if (ck::Is_NOT_Valid(_NetworkTimeSubsystem))
    { return; }

    _NetworkTimeSubsystem->OnNetworkClockSynchronized_Cpp.Remove(_DelegateHandle);
}

auto
    FProcessor_NetTimeSync_OnNetworkClockSynchronized::
    OnNetworkClockSynchronized(
        float OldServerDelta,
        float NewServerDelta,
        float RoundTripTime)
    -> void
{
    _Registry.View<TObjectPtr<UCk_Fragment_NetTimeSync_Rep>>().ForEach(
    [&](EntityType InEntity, TObjectPtr<UCk_Fragment_NetTimeSync_Rep> InRep)
    {
        InRep->DoBroadcast_NetTimeSync(FCk_Time{RoundTripTime});
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FProcessor_NetTimeSync_HandleRequests::
    Tick(
        TimeType InDeltaT)
    -> void
{
    TProcessor::Tick(InDeltaT);

    _Registry.Clear<MarkedDirtyBy>();
}

auto
    FProcessor_NetTimeSync_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_NetTimeSync_Requests& InRequests)
    -> void
{
    ck::algo::ForEachRequest(InRequests._NetTimeSyncRequests,
    [&](const FCk_Request_NetTimeSync_NewSync& InNewSync)
    {
        const auto& isNetTimeSyncEnabled = UCk_Utils_NetTimeSync_UserSettings_UE::Get_EnableNetTimeSynchronization();
        const auto& roundTripTime = isNetTimeSyncEnabled ? InNewSync.Get_RoundTripTime() : FCk_Time::ZeroSecond();

        _Registry.View<FFragment_NetTimeSync>().ForEach([&](EntityType InEntity, FFragment_NetTimeSync& InTimeSync)
        {
            InTimeSync._RoundTripTime = roundTripTime;
            InTimeSync._PlayerRoundTripTimes.FindOrAdd(InNewSync.Get_PlayerController(), roundTripTime);
        });
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FProcessor_NetTimeSync_FirstSync::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_NetTimeSync& InTimeToSyncFrom,
        const TObjectPtr<UCk_Fragment_NetTimeSync_Rep>&)
    -> void
{
    /*
     * FirstSync always runs on the Entities that have the fragments this Processor requires. It will consume
     * almost no resources if there are no Entities that have not at least synced once
     */

    _Registry.View<FFragment_NetTimeSync, ck::TExclude<FTag_NetTimeSync_SyncedAtleastOnce>>().ForEach(
    [&](EntityType InEntity, FFragment_NetTimeSync& InTimeSync)
    {
        _Registry.Add<ck::FTag_NetTimeSync_SyncedAtleastOnce>(InEntity);
        InTimeSync = InTimeToSyncFrom;
    });
}

// --------------------------------------------------------------------------------------------------------------------

}
