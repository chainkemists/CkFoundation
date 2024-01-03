#include "CkEntityBridge_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkEntityBridge/CkEntityBridge_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityBridge_HandleRequests::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            FFragment_EntityBridge_Requests& InRequests) const
        -> void
    {
        for (const auto& Request : InRequests._Requests)
        {
            const auto& EntityConfig = Request.Get_EntityConfig();

            CK_ENSURE_IF_NOT(ck::IsValid(EntityConfig),
                TEXT("EntityConfig is INVALID. Unable to handle Request for [{}]"), InHandle)
            { return; }

            const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

            if (Request.Get_PreBuildFunc())
            { Request.Get_PreBuildFunc() (NewEntity); }

            EntityConfig->Build(NewEntity);

            if (Request.Get_PostSpawnFunc())
            { Request.Get_PostSpawnFunc()(NewEntity); }

            entity_bridge::VeryVerbose(TEXT("[EntityBridge] Built new Entity [{}] from Entity Config PDA [{}] by Request from [{}]"),
                NewEntity, EntityConfig, InHandle);

            // TODO: Fire signal for new entity created once we have signals
        }

        InHandle.Remove<FFragment_EntityBridge_Requests>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
