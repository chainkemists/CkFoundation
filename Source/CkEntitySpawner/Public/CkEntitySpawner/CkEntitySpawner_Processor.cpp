#include "CkEntitySpawner_Processor.h"

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

        auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        auto ChannelResult = UCk_Utils_ActorRelay_UE::Request_AcquireChannel(World, InPending.Get_ReplicatedChannelGroup());

        // If no channel is ready yet (pool still populating, or companion entity not yet set up),
        // leave the fragment in place so MarkedDirtyBy re-fires us next tick.
        if (ck::Is_NOT_Valid(ChannelResult))
        { return; }

        auto LifetimeOwner = ChannelResult.Get_ChannelEntity();
        UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(LifetimeOwner, EntityScript, FInstancedStruct{});

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
