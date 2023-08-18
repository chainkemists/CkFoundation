#include "CkIntent_Utils.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkIntent_Fragment.h"

#include "Actor/CkActor_Utils.h"

#include "CkActor/ActorInfo/CkActorInfo_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Intent_UE::
    Request_Add(FCk_Handle InHandle)
    -> void
{
    //const auto& BasicDetails = UCk_Utils_ActorInfo_UE::Get_ActorInfoBasicDetails(InHandle);

    //const auto Intent_RO = UCk_Intent_ReplicatedObject_UE::Create(BasicDetails.Get_Actor().Get(), InHandle);
    InHandle.Add<ck::FCk_Fragment_Intent_Params>();
    InHandle.Add<ck::FTag_Intent_Setup>();
}

auto
    UCk_Utils_Intent_UE::
    Request_AddNewIntent(FCk_Handle InHandle, FGameplayTag InIntent)
    -> void
{
    auto& IntentComp = [&]() -> ck::FCk_Fragment_Intent_Requests&
    {
        if (InHandle.Has<ck::FCk_Fragment_Intent_Requests>())
        { return InHandle.Get<ck::FCk_Fragment_Intent_Requests>(); }

        return InHandle.Add<ck::FCk_Fragment_Intent_Requests>();
    }();

    IntentComp._Requests.Add(FCk_Request_Intent_NewIntent{InIntent});
}
