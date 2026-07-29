#include "CkEntitySpawner_Processor.h"

#include "CkActorRelay/CkActorRelay_Fragment_Data.h"
#include "CkActorRelay/CkActorRelay_Subsystem.h"
#include "CkActorRelay/CkActorRelay_GroupSubsystem.h"
#include "CkActorRelay/CkActorRelay_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

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

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        auto RelaySubsystem = World->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();
        if (ck::Is_NOT_Valid(RelaySubsystem))
        { return; }

        auto GroupSubsystem = RelaySubsystem->Get_GroupSubsystem(InPending.Get_ReplicatedChannelGroup());
        if (ck::Is_NOT_Valid(GroupSubsystem))
        { return; }

        const auto& SaveKeyIdentity = InPending.Get_SaveKeyIdentity();

        auto Pending = GroupSubsystem->Request_AcquireChannel();
        UCk_Utils_PendingActorRelay_UE::Promise_OnAcquired(Pending,
            [EntityScript, SaveKeyIdentity, WeakWorld = MakeWeakObjectPtr(World)](FCk_ActorRelay_ChannelResult InResult)
            {
                // Resolved at promise-fire time: the pool's pin is the root, so a script recycled
                // while the acquisition was pending leaves no spawn target and this aborts loudly.
                auto* ResolvedEntityScript = EntityScript.Get();
                CK_ENSURE_IF_NOT(ck::IsValid(ResolvedEntityScript),
                    TEXT("EntitySpawner pending spawn [SaveKey: {}]: pooled EntityScript was recycled while the relay "
                         "channel acquisition was pending — the spawn is abandoned. A keyed row waiting to adopt this "
                         "entity will never rendezvous."),
                    SaveKeyIdentity)
                { return; }

                auto LifetimeOwner = InResult.Get_ChannelEntity();

                // A KEYED spawn is a rendezvous target — during a load the saved row waits to ADOPT this very
                // entity, so it must pass the load-gate spawn suppression. An unkeyed spawn stays suppressible:
                // its saved row is recipe-respawned by the loader, and admitting the world's copy would double it.
                auto EcsWorld = static_cast<UCk_EcsWorld_Subsystem_UE*>(nullptr);
                if (NOT SaveKeyIdentity.IsEmpty() && WeakWorld.IsValid())
                { EcsWorld = WeakWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>(); }
                const auto RendezvousWindow = FCk_ScopedRendezvousSpawnWindow{EcsWorld};

                auto Pending = UCk_Utils_EntityScript_UE::Request_SpawnEntity_Archetype(
                    LifetimeOwner, ResolvedEntityScript, FInstancedStruct{}, {});

                auto EntityUnderConstruction = Pending.Get_EntityUnderConstruction();
                if (ck::Is_NOT_Valid(EntityUnderConstruction))
                { return; }

                // Without this the entity inherits the ActorRelay channel as its ContextOwner and replicates
                // that; the spawn pipeline carries the override through so the client copy resolves to self.
                UCk_Utils_ContextOwner_UE::Request_OverrideToSelf(EntityUnderConstruction, {});

                // Stamped here rather than on the queue entity: the queue entity is bookkeeping that dies this
                // tick, and this is the first moment the payload the level-placed spawner owns actually exists.
                if (NOT SaveKeyIdentity.IsEmpty())
                { ck::save_key::Assign(EntityUnderConstruction, SaveKeyIdentity); }
            });

        // The lambda carries the entity-script class forward; destroying the queue entity is what stops re-entry.
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
