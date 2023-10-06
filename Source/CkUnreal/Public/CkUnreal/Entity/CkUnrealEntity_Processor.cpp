#include "CkUnrealEntity_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkUnreal/Entity/CkUnrealEntity_Fragment.h"
#include "CkNet/CkNet_Log.h"

namespace ck
{
    auto FProcessor_UnrealEntity_HandleRequests::ForEachEntity(TimeType, HandleType InHandle,
        FCk_Fragment_UnrealEntity_Requests& InRequests) const -> void
    {
        for (const auto& Request : InRequests._Requests)
        {
            const auto& UnrealEntityPDA = Request.Get_UnrealEntity();

            CK_ENSURE_IF_NOT(ck::IsValid(UnrealEntityPDA),
                TEXT("UnrealEntityPDA is INVALID. Unable to handle Request for [{}]"), InHandle)
            { return; }

            const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

            if (Request.Get_PreBuildFunc())
            { Request.Get_PreBuildFunc() (NewEntity); }

            UnrealEntityPDA->Build(NewEntity);

            if (Request.Get_PostSpawnFunc())
            { Request.Get_PostSpawnFunc()(NewEntity); }

            net::VeryVerbose(TEXT("[UnrealEntity] Built new Unreal Entity [{}] from Unreal Entity PDA [{}] by Request from [{}]"),
                NewEntity, UnrealEntityPDA, InHandle);

            // TODO: Fire signal for new entity created once we have signals
        }

        InHandle.Remove<FCk_Fragment_UnrealEntity_Requests>();
    }
}
