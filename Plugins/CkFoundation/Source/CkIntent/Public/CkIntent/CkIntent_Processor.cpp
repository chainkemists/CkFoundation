#include "CkIntent_Processor.h"

#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

namespace ck
{
    auto
        FCk_Processor_Intent_Setup::
        Tick(FTimeType InDeltaT) -> void
    {
        // TODO: Hacking time... we need a Future
        _Delay += InDeltaT;

        if (_Delay.Get_Seconds() < 15.0f)
        { return; }

        TProcessor::Tick(InDeltaT);
    }

    auto
        FCk_Processor_Intent_Setup::
        ForEachEntity(
            FTimeType InDeltaT,
            FHandleType InHandle,
            FTag_Intent_Setup&)
    {
        auto& InIntentParams = InHandle.Get<FCk_Fragment_Intent_Params>();

        const auto& BasicDetails =  UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails(InHandle);

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
        FCk_Processor_Intent_HandleRequests::
        ForEachEntity(
            FTimeType InDeltaT,
            FHandleType InHandle,
            FCk_Fragment_Intent_Params& InParams,
            FCk_Fragment_Intent_Requests& InRequests)
    {
        // TODO: this should be handled better with Warning OR ensure OR 
        if (NOT ck::IsValid(InParams.Get_Intent_RO()))
        { return; }

        for (auto& Request : InRequests.Get_Requests())
        {
            InParams.Get_Intent_RO()->AddIntent(Request.Get_Intent());
        }

        InHandle.Remove<FCk_Fragment_Intent_Requests>();
    }
}
