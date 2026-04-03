#include "CkActorRelay_Fragment_Data.h"

#include "CkActorRelay_Actor.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(FCk_ActorRelay_ChannelResult, IsValid_Policy_Default, [=](const FCk_ActorRelay_ChannelResult& InResult)
{
    return InResult.Get_ChannelActor().IsValid() && ck::IsValid(InResult.Get_ChannelEntity());
});

// --------------------------------------------------------------------------------------------------------------------
