#include "CkIntent_Processor.h"

#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

namespace ck
{
    auto
        FCk_Processor_Intent_Setup::
        ForEachEntity(
            FTimeType InDeltaT,
            FHandleType InHandle,
            FCk_Fragment_Intent_Params& InIntentParams)
    {
        const auto& BasicDetails =  UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails(InHandle);
        const auto Intent_RO = UCk_Intent_ReplicatedObject_UE::Create(BasicDetails.Get_Actor().Get(), InHandle);
        InIntentParams._Intent_RO = Intent_RO;

        InHandle.Remove<FTag_Intent_Setup>();
    }
}
