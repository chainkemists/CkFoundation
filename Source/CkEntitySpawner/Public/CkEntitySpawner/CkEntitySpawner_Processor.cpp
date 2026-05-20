#include "CkEntitySpawner_Processor.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"
#include "CkActorRelay/CkActorRelay_Subsystem.h"
#include "CkActorRelay/CkActorRelay_GroupSubsystem.h"
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

        auto RelaySubsystem = World->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();
        if (ck::Is_NOT_Valid(RelaySubsystem))
        { return; }

        auto GroupSubsystem = RelaySubsystem->Get_GroupSubsystem(InPending.Get_ReplicatedChannelGroup());
        if (ck::Is_NOT_Valid(GroupSubsystem))
        { return; }

        auto Pending = GroupSubsystem->Request_AcquireChannel();
        UCk_Utils_PendingActorRelay_UE::Promise_OnAcquired(Pending,
            [EntityScript](FCk_ActorRelay_ChannelResult InResult)
            {
                auto LifetimeOwner = InResult.Get_ChannelEntity();
                UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(
                    LifetimeOwner, EntityScript, FInstancedStruct{});
            });

        // The lambda above carries the entity-script class forward. The
        // fragment-bearing entity has served its purpose — destroy it so we
        // don't re-enter this processor next tick.
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
