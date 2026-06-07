#include "CkActorRespawn_Processor.h"

#include "CkActorRespawn_Fragment.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"   // Get_WorldForEntity
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h"
#include "CkEcsExt/OwningActor/CkActorRebind_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"          // Has + Get_EntityCurrentTransform

#include <Engine/World.h>
#include <GameFramework/Actor.h>
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
        }

        _PendingRespawns.Reset();
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
