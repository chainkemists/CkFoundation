#include "CkActorRespawn_Processor.h"

#include "CkActorRespawn_Fragment.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"   // Get_WorldForEntity
#include "CkEcs/EntityScript/CkEntityScript.h"             // UCk_EntityScript_UE (complete type)
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"    // FFragment_EntityScript_Current
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"       // Request_ReplicateEntityScript (shared establish)
#include "CkEcs/Net/CkNet_Utils.h"                         // UCk_Utils_Net_UE
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h" // UCk_Fragment_EntityReplicationDriver_Rep
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"    // Has
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/EntityScript/CkEntityScript_WithActor_Data.h" // FCk_EntityScript_WithActor_SpawnParams
#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h"
#include "CkEcsExt/OwningActor/CkActorRebind_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"          // Has + Get_EntityCurrentTransform

#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <StructUtils/InstancedStruct.h>
#include "UObject/SoftObjectPath.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ActorRespawn);

namespace ck
{
    auto
        FProcessor_ActorRespawn::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _PendingRespawns.Reset();

        // Runs the view iteration → ForEachEntity, which collects the MARKED entities into _PendingRespawns. The
        // iteration finishes BEFORE any spawning so we never mutate the registry mid-view.
        Super::DoTick(InDeltaT);

        if (_PendingRespawns.IsEmpty())
        { return; }

        // Resolve the world ONCE from the processor's transient. After the post-restore processor-graph rebuild
        // (CkSnapshot triggers it), this processor was re-created against the restored registry, so _TransientEntity
        // is the LIVE adopted transient — which carries the world-fragment re-attached by
        // Request_AdoptRestoredTransient. (Resolving per restored gameplay entity instead is unreliable: their
        // lifetime chains may not walk back to a world after restore. The transient reads the world-fragment directly.)
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);
        if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
        {
            ck::ecs::Warning(TEXT("[M2b2a] respawn — processor transient has no resolvable World; deferring [{}] respawn(s)"),
                _PendingRespawns.Num());
            return;
        }

        for (auto& Entity : _PendingRespawns)
        {
            if (ck::Is_NOT_Valid(Entity) || NOT Entity.Has<FFragment_ActorSpawnIntent>())
            { continue; }

            // Consume the marker up-front so a re-entrant tick / pump never double-spawns this entity.
            Entity.Clear<FTag_ActorRespawn_Pending>();

            const auto& ActorClassPath = Entity.Get<FFragment_ActorSpawnIntent>().Get_ActorClassPath();
            auto* ActorClass = FSoftClassPath{ActorClassPath}.TryLoadClass<AActor>();
            if (ActorClass == nullptr)
            {
                ck::ecs::Warning(TEXT("[M2b2a] respawn — entity [{}] actor class path [{}] unloadable; skipped"),
                    Entity, ActorClassPath);
                continue;
            }

            const auto SpawnTransform = UCk_Utils_Transform_UE::Has(Entity)
                ? UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Entity)
                : FTransform::Identity;

            auto SpawnInfo = FActorSpawnParameters{};
            SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            auto* Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnInfo);
            if (Actor == nullptr)
            {
                ck::ecs::Warning(TEXT("[M2b2a] respawn — SpawnActor failed for entity [{}]"), Entity);
                continue;
            }

            ck::ecs::Display(TEXT("[M2b2a] respawn — entity [{}] spawned + re-bridging actor of class [{}]"),
                Entity, ActorClassPath);

            // Driver re-establish happens INSIDE this scheduler tick (FGroup_Gameplay_Script) — safe.
            UCk_Utils_ActorRebind_UE::Request_RebindActor(Entity, Actor);

            // Re-issue the entity-script replication so the re-established driver actually pushes the entity to
            // clients (the rebind only re-adds the driver fragment; see DoReplicate_RestoredEntity).
            DoReplicate_RestoredEntity(Entity, Actor);
        }

        _PendingRespawns.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ActorRespawn::
        DoReplicate_RestoredEntity(
            FCk_Handle& InEntity,
            AActor* InActor)
        -> void
    {
        // Gate: only replicated entities on the networked authority need re-replication. Non-replicated / client /
        // non-net entities legitimately do nothing here.
        if (NOT UCk_Utils_Net_UE::Has(InEntity))
        { return; }

        if (UCk_Utils_Net_UE::Get_Replication(InEntity) != ECk_Replication::Replicates)
        { return; }

        // EFFECTIVE replication gate (mirrors UCk_EntityScript_WithActor_UE::Get_EffectiveReplication): a WithActor
        // entity whose bridged actor does NOT replicate reports Replicates via its net-params fragment but never pushes
        // to the wire (no driver is ever created). Use the actor's actual replication — NOT the net-params value — so
        // these entities are skipped cleanly instead of tripping the "no driver" invariant below. The two values can
        // disagree (net-params is declared per-entity-script; effective is downgraded by the actor).
        if (NOT InActor->GetIsReplicated())
        { return; }

        if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InEntity))
        { return; }

        // #2 invariant — past the gate the entity MUST be re-replicable. A silent skip here is exactly the M2b-2b bug:
        // clients receive the respawned (independently-replicating) actor but never the ECS entity, so no bridge and no
        // fragments. Surface each precondition LOUDLY instead of returning quietly.
        CK_ENSURE_IF_NOT(InEntity.Has<FFragment_EntityScript_Current>(),
            TEXT("Restored replicated entity [{}] has no EntityScript fragment — cannot re-replicate; clients will get "
                 "the respawned actor but never the ECS entity"), InEntity)
        { return; }

        CK_ENSURE_IF_NOT(UCk_Utils_EntityReplicationDriver_UE::Has(InEntity),
            TEXT("Restored replicated entity [{}] has no ReplicationDriver after rebind — cannot re-replicate to clients"),
            InEntity)
        { return; }

        // Snapshot-restored: FFragment_EntityScript_Current is snapshotable and round-trips the script instance via
        // its class-path, so the restored entity carries its EntityScript — the source of the class we replicate.
        const auto& Script = InEntity.Get<FFragment_EntityScript_Current>().Get_Script();
        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Restored replicated entity [{}] has an invalid EntityScript instance — cannot re-replicate to clients"),
            InEntity)
        { return; }

        // WithActor spawn params reference the freshly respawned actor. The replicated reconstitution payload carries
        // these so the client matches the replicated entity to its own replicated actor copy (Get_AreSpawnParamsMatching).
        auto SpawnParams = FInstancedStruct::Make<FCk_EntityScript_WithActor_SpawnParams>(InActor);

        ck::ecs::Display(TEXT("[M2b] DoReplicate_RestoredEntity: re-issuing replicate for Entity=[{}] Script=[{}] Actor=[{}]"),
            InEntity, Script.Get(), InActor->GetName());

        // SHARED establishment with the fresh-spawn replicate processor (FProcessor_EntityScript_Replicate) — see
        // UCk_Utils_EntityScript_UE::Request_ReplicateEntityScript. A WithActor entity is self-owned, so the replicated
        // owner is the entity itself. We call it directly (not via FRequest_EntityScript_Replicate) because that
        // processor's view also requires FTag_EntityScript_FinishConstruction, which a RESTORED entity no longer
        // carries (construction completed pre-save), so an enqueued request would never be processed.
        UCk_Utils_EntityScript_UE::Request_ReplicateEntityScript(InEntity, InEntity, Script.Get(), SpawnParams);

        // #2 invariant — by the end of the respawn frame the driver MUST carry a populated ReplicationData_EntityScript
        // (the payload Iris pushes). If it is still empty the client silently never materialises the entity; catch that
        // regression here, loudly, instead of in a playtest.
        const auto& Driver = InEntity.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
        CK_ENSURE_IF_NOT(ck::IsValid(Driver) &&
            ck::IsValid(Driver->Get_ReplicationData_EntityScript().Get_EntityScriptClass().Get(), ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Restored replicated entity [{}] has an empty ReplicationData_EntityScript after re-replicate — "
                 "clients will NOT receive the entity"), InEntity)
        { return; }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ActorRespawn::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_ActorSpawnIntent& /*InIntent*/)
        -> void
    {
        // Iterate the clean data fragment; collect only the entities the snapshot restore marked for respawn.
        if (NOT InHandle.Has<FTag_ActorRespawn_Pending>())
        { return; }

        _PendingRespawns.Emplace(InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
