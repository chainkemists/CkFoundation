#include "CkIntent_Processor.h"

#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

namespace ck
{
    auto
        FCk_Processor_Intent_Setup::
        Tick(FTimeType InDeltaT) -> void
    {
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
        if (NOT ck::IsValid(InParams.Get_Intent_RO()))
        { return; }

        for (auto& Request : InRequests.Get_Requests())
        {
            InParams.Get_Intent_RO()->AddIntent(Request.Get_Intent());
        }

        InHandle.Remove<FCk_Fragment_Intent_Requests>();
    }
}
