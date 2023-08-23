#include "CkActorInfo_Processors.h"

#include "CkActor/CkActor_Log.h"

#include "Kismet/KismetSystemLibrary.h"

namespace ck
{
    auto
        FCk_Processor_ActorInfo_LinkUp::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FTag_ActorInfo_LinkUp& InTag)
    {
        const auto& ActorInfo = InHandle.Get<FCk_Fragment_ActorInfo_Current>();
        if (UKismetSystemLibrary::IsDedicatedServer(ActorInfo.Get_EntityActor().Get()))
        {
            return;
        }

        // TODO: add checks and make this safer
        const auto ActorInfoComp = ActorInfo.Get_EntityActor()->GetComponentByClass<UCk_ActorInfo_ActorComponent_UE>();

        if (ck::Is_NOT_Valid(ActorInfoComp))
        { return; }

        ActorInfoComp->_EntityHandle = InHandle;

        ck::actor::Log(TEXT("Handle [{}] processed for LinkUp"), InHandle);
        InHandle.Remove<FTag_ActorInfo_LinkUp>();
    }
}
