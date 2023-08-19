#include "CkUnrealEntity_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkUnreal/CkUnreal_Log.h"

namespace ck
{
    auto FCk_Processor_UnrealEntity_HandleRequests::ForEachEntity(FTimeType, FCk_Handle InHandle,
        FCk_Fragment_UnrealEntity_Requests& InRequests) const -> void
    {
        for (const auto& Request : InRequests._Requests)
        {
            const auto& UnrealEntityPDA = Request.Get_UnrealEntity();

            CK_ENSURE_IF_NOT(ck::IsValid(UnrealEntityPDA),
                TEXT("UnrealEntityPDA is INVALID. Unable to handle Request for [{}]"), InHandle)
            { return; }

            const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(**InHandle);

            if (Request.Get_PreBuildFunc())
            { Request.Get_PreBuildFunc() (NewEntity); }

            std::ignore = UnrealEntityPDA->Build(NewEntity);

            if (Request.Get_PostSpawnFunc())
            { Request.Get_PostSpawnFunc()(NewEntity); }

            unreal::VeryVerbose(TEXT("[UnrealEntity] Built new Unreal Entity [{}] from Unreal Entity PDA [{}] by Request from [{}]"),
                NewEntity, UnrealEntityPDA, InHandle);

            // TODO: Fire signal for new entity created once we have signals
        }

        InHandle.Remove<FCk_Fragment_UnrealEntity_Requests>();
    }
}
