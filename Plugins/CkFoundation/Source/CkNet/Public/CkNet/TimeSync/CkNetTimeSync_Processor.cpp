#include "CkNetTimeSync_Processor.h"

#include "NetworkTimeSubsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Processor_TimeSync_OnNetworkClockSynchronized::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const TObjectPtr<UCk_Fragment_TimeSync_Rep>& InTimeSync_Rep)
{
    if (_DelegateHandle.IsValid())
    { return; }

    _NetworkTimeSubsystem = UNetworkTimeSubsystem::Get(InTimeSync_Rep.Get());

    CK_ENSURE_IF_NOT(ck::IsValid(_NetworkTimeSubsystem),
        TEXT("Could not get a valid NetworkTimeSubsystem from [{}] on Entity [{}]"), InTimeSync_Rep, InHandle)
    { return; }

    _DelegateHandle = _NetworkTimeSubsystem->OnNetworkClockSynchronized_Cpp.AddRaw(
        this, &FCk_Processor_TimeSync_OnNetworkClockSynchronized::OnNetworkClockSynchronized);
}

FCk_Processor_TimeSync_OnNetworkClockSynchronized::
    ~FCk_Processor_TimeSync_OnNetworkClockSynchronized()
{
    if (NOT _DelegateHandle.IsValid())
    { return; }

    if (ck::Is_NOT_Valid(_NetworkTimeSubsystem))
    { return; }

    _NetworkTimeSubsystem->OnNetworkClockSynchronized_Cpp.Remove(_DelegateHandle);
}

auto
    FCk_Processor_TimeSync_OnNetworkClockSynchronized::
    OnNetworkClockSynchronized(
        float OldServerDelta,
        float NewServerDelta,
        float RoundTripTime)
    -> void
{
    _Registry.View<TObjectPtr<UCk_Fragment_TimeSync_Rep>>().ForEach(
    [&](EntityType InEntity, TObjectPtr<UCk_Fragment_TimeSync_Rep> InRep)
    {
        auto Handle = HandleType{InEntity, _Registry};
        InRep->DoBroadcast_TimeSync(RoundTripTime);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Processor_TimeSync_HandleRequests::
    Tick(
        TimeType InDeltaT)
    -> void
{
    TProcessor::Tick(InDeltaT);

    _Registry.Clear<MarkedDirtyBy>();
}

auto
    FCk_Processor_TimeSync_HandleRequests::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        FFragment_TimeSync_Requests& InRequests)
{
    ck::algo::ForEachRequest(InRequests._TimeSyncRequests,
    [&](const FCk_Request_NetTimeSync_NewSync& InNewSync)
    {
        _Registry.View<FFragment_TimeSync>().ForEach([&](EntityType InEntity, FFragment_TimeSync& InTimeSync)
        {
            InTimeSync._RoundTripTime = InNewSync.Get_RoundTripTime();
            InTimeSync._PlayerRoundTripTimes.FindOrAdd(InNewSync.Get_PlayerController(), InNewSync.Get_RoundTripTime());
        });
    });
}

// --------------------------------------------------------------------------------------------------------------------


}
