#include "CkIntent_Processor.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Intent_Setup::
        Tick(TimeType InDeltaT) -> void
    {
        // TODO: Hacking time... we need a Future
        _Delay += InDeltaT;

        if (_Delay.Get_Seconds() < 15.0f)
        { return; }

        TProcessor::Tick(InDeltaT);
    }

    auto
        FProcessor_Intent_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FTag_Intent_Setup&)
    {
        auto& InIntentParams = InHandle.Get<FFragment_Intent_Params>();

        const auto& BasicDetails =  UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);

        if (NOT ck::IsValid(BasicDetails.Get_Actor()))
        { return; }

        // TODO: we need to have better patterns so that this kind of code has a standard pattern
        if (NOT BasicDetails.Get_Actor()->HasAuthority())
        {
            InHandle.Remove<FTag_Intent_Setup>();
            return;
        }

        const auto Intent_RO = UCk_Intent_ReplicatedObject_UE::Create(BasicDetails.Get_Actor().Get(), InHandle);
        InIntentParams._Intent_RO = Intent_RO;
        InIntentParams._Intent_RO->_Ready = true;

        InHandle.Remove<FTag_Intent_Setup>();
    }

    auto
        FProcessor_Intent_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Intent_Params& InParams,
            FFragment_Intent_Requests& InRequests)
    {
        // TODO: this should be handled better with Warning OR ensure OR
        if (NOT ck::IsValid(InParams.Get_Intent_RO()))
        { return; }

        for (auto& Request : InRequests.Get_Requests())
        {
            InParams.Get_Intent_RO()->AddIntent(Request.Get_Intent());
        }

        InHandle.Remove<FFragment_Intent_Requests>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
