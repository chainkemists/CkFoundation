#include "CkEntitySpawner_Processor.h"

#include "CkEntitySpawner/CkEntitySpawner_Log.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"
#include "CkActorRelay/CkActorRelay_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_EntitySpawner_Spawn);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntitySpawner_Spawn::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntitySpawner_PendingSpawn& InPending)
        -> void
    {
        const auto& EntityScript = InPending.Get_EntityScript();

        if (ck::Is_NOT_Valid(EntityScript))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        const auto& ChannelGroup = InPending.Get_ReplicatedChannelGroup();
        const auto IsReplicated = ChannelGroup.IsValid();

        auto LifetimeOwner = [&]() -> FCk_Handle
        {
            if (NOT IsReplicated)
            { return UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle); }

            auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            auto ChannelResult = UCk_Utils_ActorRelay_UE::Request_AcquireChannel(World, ChannelGroup);

            if (ck::Is_NOT_Valid(ChannelResult))
            { return {}; }

            return ChannelResult.Get_ChannelEntity();
        }();

        if (ck::Is_NOT_Valid(LifetimeOwner))
        { return; }

        UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(LifetimeOwner, EntityScript, FInstancedStruct{});

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
