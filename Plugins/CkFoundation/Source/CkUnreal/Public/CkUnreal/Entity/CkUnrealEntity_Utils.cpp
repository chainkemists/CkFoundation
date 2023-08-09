#include "CkUnrealEntity_Utils.h"

#include "CkUnrealEntity_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_UnrealEntity_UE::
Request_Spawn(FCk_Handle InHandle, const FCk_Request_UnrealEntity_Spawn& InRequest) -> void
{
    auto& RequestComp = InHandle.Add<ck::FCk_Fragment_UnrealEntity_Requests>();
    RequestComp._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------
