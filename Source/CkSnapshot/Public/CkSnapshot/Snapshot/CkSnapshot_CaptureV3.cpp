#include "CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/SaveKey/CkSnapshot_SaveKey_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"                 // ck::MakeHandle
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // FFragment_LifetimeOwner, FTag_ConstructSpawned
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"    // Get_LifetimeOwner
#include "CkEcs/EntityScript/CkEntityScript.h"           // UCk_EntityScript_UE
#include "CkEcs/EntityScript/CkEntityScript_SpawnRecipe.h" // FFragment_SpawnRecipe
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_BuildRecipe.h" // FFragment_BuildRecipe (DefinitionBuilt)
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h" // handler registry (Produce)
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h" // FFragment_ActorSpawnIntent
#include "CkEcsExt/Transform/CkTransform_Utils.h"             // bridged-actor spawn transform

#include "Misc/EngineVersion.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_capturev3
    {
        constexpr auto k_NoEntity = 0xFFFFFFFFu;

        auto
            Get_SavedId(
                const FCk_Handle& InHandle)
            -> uint32
        {
            return ck::IsValid(InHandle) ? static_cast<uint32>(InHandle.Get_Entity().Get_ID()) : k_NoEntity;
        }

        auto
            IsMarkedForDestruction(
                const FCk_Handle& InHandle)
            -> bool
        {
            return InHandle.Has_Any<
                ck::FTag_DestroyEntity_Initiate,
                ck::FTag_DestroyEntity_EndPlay,
                ck::FTag_DestroyEntity_Teardown,
                ck::FTag_DestroyEntity_Await,
                ck::FTag_DestroyEntity_Finalize>();
        }

        // Depth in the lifetime tree (hops to a root/transient owner). Used to order owners before dependents so
        // the entity table is topology-sorted. Cycle-guarded.
        auto
            DoGet_LifetimeDepth(
                const FCk_Handle& InHandle)
            -> int32
        {
            constexpr auto MaxDepth = 256;
            auto Depth   = 0;
            auto Current = InHandle;

            while (Depth < MaxDepth && Current.Has<ck::FFragment_LifetimeOwner>())
            {
                auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Current);
                if (ck::Is_NOT_Valid(Owner) || Owner == Current)
                { break; }
                Current = Owner;
                ++Depth;
            }
            return Depth;
        }

        // Serialize an FInstancedStruct (spawn params or a Produce payload) to a byte blob in two steps: first
        // tagged-property data via a name-as-string proxy (object refs by path, Transient handle fields skipped),
        // then the shared handle walker writes each FCk_Handle's raw saved entity id.
        auto
            SerializeInstancedStruct(
                const FInstancedStruct& InStruct)
            -> TArray<uint8>
        {
            auto Blob = TArray<uint8>{};
            if (InStruct.GetScriptStruct() == nullptr)
            { return Blob; }

            auto MemoryWriter = FMemoryWriter{Blob, /*bIsPersistent=*/true};
            constexpr auto LoadIfFindFails = true;
            auto Proxy = FObjectAndNameAsStringProxyArchive{MemoryWriter, LoadIfFindFails};
            Proxy.ArIsSaveGame = false;      // false ⇒ capture every non-Transient UPROPERTY, regardless of CPF_SaveGame
            Proxy.SetIsPersistent(true);

            auto Context = ck::FSnapshotContext{}; // save-mode handle write (raw id); no loader remap on save

            auto Copy = FInstancedStruct{InStruct};
            Copy.Serialize(Proxy);
            ck::snapshot::RemapHandles(Copy.GetScriptStruct(), Copy.GetMutableMemory(), Proxy, Context);
            return Blob;
        }

        // If the entity's owning actor is a player pawn / controller / state, return a stable rendezvous string
        // (the PlayerState unique-id; empty for standalone player 0). Unset ⇒ not a player-owned entity.
        auto
            TryResolve_PlayerRendezvous(
                const FCk_Handle& InHandle)
            -> TOptional<FString>
        {
            const auto* Actor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(InHandle);
            if (Actor == nullptr)
            { return {}; }

            const APlayerState* PlayerState = nullptr;
            if (const auto* AsPlayerState = Cast<APlayerState>(Actor))
            { PlayerState = AsPlayerState; }
            else if (const auto* AsController = Cast<APlayerController>(Actor))
            { PlayerState = AsController->PlayerState; }
            else if (const auto* AsPawn = Cast<APawn>(Actor); AsPawn != nullptr && AsPawn->IsPlayerControlled())
            { PlayerState = AsPawn->GetPlayerState(); }

            if (PlayerState == nullptr)
            {
                // Not resolvable to a PlayerState but IS a player-controlled actor ⇒ standalone player 0 (empty id).
                if (const auto* AsPawn = Cast<APawn>(Actor); AsPawn != nullptr && AsPawn->IsPlayerControlled())
                { return FString{}; }
                return {};
            }

            const auto UniqueId = PlayerState->GetUniqueId();
            return UniqueId.IsValid() ? UniqueId.ToString() : FString{};
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Run_CaptureV3_Registry(
            ck::SnapshotRegistryType& InRegistry,
            FCk_RegistryHandle InRegistryHandle,
            UWorld* InWorldOrNull,
            FArchive& InByteWriter,
            FCk_Snapshot_HeaderV3& InOutHeader)
        -> ECk_SnapshotResult
    {
        using namespace ck_snapshot_capturev3;

        auto CkRegistry = FCk_Registry{InRegistryHandle};

        // ---- Collect candidate entities (every entity carrying ≥1 fragment/tag) -----------------------------------
        // Sweep the storages once and accumulate entity ids. Truly
        // empty entities never appear — they are anonymous scratch (rule 5) and would be skipped anyway.
        const auto EntityStorageHash = static_cast<uint32>(entt::type_hash<ck::SnapshotEntityType>::value());
        auto CandidateIds = TSet<uint32>{};
        for (auto&& StoragePair : InRegistry.storage())
        {
            const auto TypeHash = static_cast<uint32>(StoragePair.second.info().hash());
            if (TypeHash == EntityStorageHash)
            { continue; }

            for (const auto Entity : StoragePair.second)
            { CandidateIds.Add(static_cast<uint32>(Entity)); }
        }

        // Resolve the transient so it is never persisted (bookkeeping, not world state).
        auto TransientId = TOptional<uint32>{};
        if (const auto* TransientCtx = InRegistry.ctx().find<const ck::FCtx_TransientEntity>())
        { TransientId = static_cast<uint32>(TransientCtx->Entity.Get_ID()); }

        // ---- Pass 1: classify. Determine the persisted set + provenance + ordering. --------------------------------
        struct FClassified
        {
            FCk_Handle                     _Handle;
            uint32                         _SavedId = k_NoEntity;
            ECk_Snapshot_V3_Provenance     _Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
            int32                          _Depth = 0;
        };

        const auto SaveTypes = FCk_PersistenceHandlerRegistry::Get_SaveHandlerTypes();

        const auto DoAnyProduce = [&](FCk_Handle& InEntity) -> bool
        {
            for (const auto* Type : SaveTypes)
            {
                const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Type);
                if (Handler == nullptr || NOT Handler->Produce)
                { continue; }
                if (Handler->Produce(InEntity).IsSet())
                { return true; }
            }
            return false;
        };

        auto Classified = TArray<FClassified>{};
        auto PersistedIds = TSet<uint32>{};
        auto AnonymousSkipped = 0;
        auto UnlabeledSkipped = 0;
        auto UnlabeledWithPayloadAudit = 0;

        for (const auto RawId : CandidateIds)
        {
            if (TransientId.IsSet() && RawId == TransientId.GetValue())
            { continue; }

            auto Handle = ck::MakeHandle(FCk_Entity{static_cast<ck::SnapshotEntityType>(RawId)}, CkRegistry);
            if (ck::Is_NOT_Valid(Handle))
            { continue; }

            // Rule 1 — pending destruction ⇒ skip.
            if (IsMarkedForDestruction(Handle))
            { continue; }

            auto Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
            auto bPersist = false;

            // Rule 2 — EngineOwned: a SaveKey (level actor) or a player pawn/controller/state.
            if (Handle.Has<FFragment_SaveKey>() ||
                (InWorldOrNull != nullptr && TryResolve_PlayerRendezvous(Handle).IsSet()))
            {
                Provenance = ECk_Snapshot_V3_Provenance::EngineOwned;
                bPersist = true;
            }
            // Rule 3 — ConstructSpawned: only if it has a real (named) label; unlabeled ⇒ save-transient (skip).
            else if (Handle.Has<ck::FTag_ConstructSpawned>())
            {
                const auto bLabeled = UCk_Utils_GameplayLabel_UE::Has(Handle)
                    && NOT UCk_Utils_GameplayLabel_UE::Get_IsUnnamedLabel(Handle);

                if (bLabeled)
                {
                    Provenance = ECk_Snapshot_V3_Provenance::ConstructSpawned;
                    bPersist = true;
                }
                else
                {
                    ++UnlabeledSkipped;
                    if (DoAnyProduce(Handle))
                    {
                        ++UnlabeledWithPayloadAudit;
                        // Guard Get_LifetimeOwner: a registry-rooted entity legitimately has no lifetime owner (else
                        // Get_LifetimeOwner ensures). Mirrors the guarded reads at the LifetimeOwnerSavedId capture below.
                        const auto OwnerId = Handle.Has<ck::FFragment_LifetimeOwner>()
                            ? Get_SavedId(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Handle))
                            : k_NoEntity;
                        ck::snapshot::Warning(
                            TEXT("v3 capture AUDIT: unlabeled ConstructSpawned child [{}] (owner saved-id [{}]) carries a "
                                 "hydration payload that will be DROPPED — it is save-transient. Give the child a "
                                 "GameplayLabel under its owner to persist it."),
                            Handle, OwnerId);
                    }
                }
            }
            // Rule 4 — RuntimeSpawned: has a retained EntityScript spawn recipe.
            else if (Handle.Has<ck::FFragment_SpawnRecipe>())
            {
                Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
                bPersist = true;
            }
            // Rule 4.5 — DefinitionBuilt: built via Request_BuildAndReplicate with a retained construction recipe.
            else if (Handle.Has<ck::FFragment_BuildRecipe>())
            {
                Provenance = ECk_Snapshot_V3_Provenance::DefinitionBuilt;
                bPersist = true;
            }
            // Rule 5 — anonymous scratch: skip, count.
            else
            {
                ++AnonymousSkipped;
            }

            if (NOT bPersist)
            { continue; }

            PersistedIds.Add(RawId);
            Classified.Add(FClassified{Handle, RawId, Provenance, DoGet_LifetimeDepth(Handle)});
        }

        // ---- Order owners before dependents (lifetime-topology; ties by saved-id for determinism) ------------------
        Classified.Sort([](const FClassified& InA, const FClassified& InB) -> bool
        {
            return InA._Depth != InB._Depth ? InA._Depth < InB._Depth : InA._SavedId < InB._SavedId;
        });

        // ---- Pass 2: build the entity + payload tables -------------------------------------------------------------
        auto Tables = FCk_Snapshot_V3_Tables{};
        auto& Entities = Tables.Get_Entities();
        auto& Payloads = Tables.Get_Payloads();

        auto EngineOwnedCount = 0;
        auto ConstructSpawnedCount = 0;
        auto RuntimeSpawnedCount = 0;
        auto DefinitionBuiltCount = 0;

        for (auto& Item : Classified)
        {
            auto Handle = Item._Handle;

            auto Entry = FCk_Snapshot_V3_EntityEntry{};
            Entry.Set_SavedId(Item._SavedId);
            Entry.Set_Provenance(Item._Provenance);
            Entry.Set_LifetimeOwnerSavedId(
                Handle.Has<ck::FFragment_LifetimeOwner>()
                    ? Get_SavedId(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Handle))
                    : k_NoEntity);

            switch (Item._Provenance)
            {
                case ECk_Snapshot_V3_Provenance::EngineOwned:
                {
                    ++EngineOwnedCount;
                    if (Handle.Has<FFragment_SaveKey>())
                    { Entry.Set_SaveKey(Handle.Get<FFragment_SaveKey>().Get_Key()); }
                    else if (const auto PlayerId = TryResolve_PlayerRendezvous(Handle); PlayerId.IsSet())
                    { Entry.Set_PlayerId(PlayerId.GetValue()); }
                    break;
                }
                case ECk_Snapshot_V3_Provenance::ConstructSpawned:
                {
                    ++ConstructSpawnedCount;
                    Entry.Set_Label(UCk_Utils_GameplayLabel_UE::Get_Label(Handle).ToString());
                    break;
                }
                case ECk_Snapshot_V3_Provenance::RuntimeSpawned:
                {
                    ++RuntimeSpawnedCount;
                    const auto& Recipe = Handle.Get<ck::FFragment_SpawnRecipe>();
                    if (const auto ScriptClass = Recipe.Get_ScriptClass(); ck::IsValid(ScriptClass.Get()))
                    { Entry.Set_ScriptClassPath(ScriptClass->GetPathName()); }

                    // Forward/cross handle refs inside spawn params are unsupported v3 — a params handle must
                    // reference a persisted entity, else it dangles on load. Loudly flag, then still write it.
                    auto ParamsCopy = FInstancedStruct{Recipe.Get_SpawnParams()};
                    if (ParamsCopy.GetScriptStruct() != nullptr)
                    {
                        ck::snapshot::ForEachHandle(ParamsCopy.GetScriptStruct(), ParamsCopy.GetMutableMemory(),
                            [&](FCk_Handle& InParamHandle) -> void
                            {
                                if (ck::Is_NOT_Valid(InParamHandle))
                                { return; }
                                const auto RefId = static_cast<uint32>(InParamHandle.Get_Entity().Get_ID());
                                CK_ENSURE_IF_NOT(PersistedIds.Contains(RefId),
                                    TEXT("v3 capture: RuntimeSpawned entity [{}] spawn params reference entity [{}] which "
                                         "is NOT persisted — it will dangle on load. Handle refs in spawn params must "
                                         "target a persisted entity."),
                                    Handle, InParamHandle) {}
                            });
                    }
                    Entry.Set_SpawnParamsBytes(SerializeInstancedStruct(Recipe.Get_SpawnParams()));

                    if (UCk_Utils_ContextOwner_UE::Has(Handle))
                    { Entry.Set_ContextOwnerSavedId(Get_SavedId(UCk_Utils_ContextOwner_UE::Get_ContextOwner(Handle))); }

                    if (Handle.Has<FFragment_ActorSpawnIntent>())
                    { Entry.Set_ActorClassPath(Handle.Get<FFragment_ActorSpawnIntent>().Get_ActorClassPath()); }

                    // Bridged actors respawn actor-first: capture the world transform so the loader
                    // spawns the actor at its saved position (WithActor::Construct then seeds the entity Transform
                    // from the actor). Same Has/Get_EntityCurrentTransform guard the retired FProcessor_ActorRespawn used.
                    if (UCk_Utils_Transform_UE::Has(Handle))
                    { Entry.Set_ActorSpawnTransform(UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Handle)); }
                    break;
                }
                case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                {
                    ++DefinitionBuiltCount;

                    // Capture each construction step by path so the loader re-creates the entity via
                    // Request_BuildAndReplicate. The entity's own fragment state (tags, spatial placement) rides its
                    // per-feature Produce below; per-instance child-attribute state (e.g. an item stack count, held on
                    // a separate anonymous attribute entity) does not persist.
                    auto Steps = TArray<FCk_Snapshot_V3_BuildStep>{};
                    for (const auto& Info : Handle.Get<ck::FFragment_BuildRecipe>().Get_ConstructionInfos())
                    {
                        auto Step = FCk_Snapshot_V3_BuildStep{};
                        if (const auto ScriptClass = Info.Get_ConstructionScript(); ck::IsValid(ScriptClass.Get()))
                        { Step.Set_ScriptClassPath(ScriptClass->GetPathName()); }
                        if (const auto* Archetype = Info.Get_ConstructionScriptArchetype().Get();
                            ck::IsValid(Archetype, ck::IsValid_Policy_NullptrOnly{}))
                        { Step.Set_ArchetypePath(Archetype->GetPathName()); }
                        Steps.Emplace(MoveTemp(Step));
                    }
                    Entry.Set_BuildRecipe(MoveTemp(Steps));

                    // Built under its context owner (the driver-bearing subject) — the loader rebuilds under the same.
                    if (UCk_Utils_ContextOwner_UE::Has(Handle))
                    { Entry.Set_ContextOwnerSavedId(Get_SavedId(UCk_Utils_ContextOwner_UE::Get_ContextOwner(Handle))); }
                    break;
                }
            }

            // G1 Transform parity — capture the CURRENT world transform for EVERY persisted entity that carries a
            // Transform fragment, regardless of provenance, so the loader restores its post-settle world position.
            // Orthogonal to the RuntimeSpawned _ActorSpawnTransform seed above (which only positions the bridged actor
            // spawn); this column corrects post-spawn drift for everyone. Identity (default) when no Transform fragment.
            if (UCk_Utils_Transform_UE::Has(Handle))
            { Entry.Set_SavedWorldTransform(UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Handle)); }

            Entities.Emplace(MoveTemp(Entry));

            // ---- Payloads: every Save-flagged handler's Produce for this entity ----
            for (const auto* Type : SaveTypes)
            {
                const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Type);
                if (Handler == nullptr || NOT Handler->Produce)
                { continue; }

                auto Produced = Handler->Produce(Handle);
                if (NOT Produced.IsSet())
                { continue; }

                auto PayloadEntry = FCk_Snapshot_V3_PayloadEntry{};
                PayloadEntry.Set_OwnerSavedId(Item._SavedId);
                PayloadEntry.Set_TypePath(Type->GetPathName());
                PayloadEntry.Set_PayloadBytes(SerializeInstancedStruct(Produced.GetValue()));
                Payloads.Emplace(MoveTemp(PayloadEntry));
            }
        }

        // ---- Serialize the tables + stamp the header census --------------------------------------------------------
        FCk_Snapshot_V3_Tables::StaticStruct()->SerializeItem(InByteWriter, &Tables, /*Defaults=*/nullptr);

        InOutHeader.Set_FormatVersion(FCk_Snapshot_HeaderV3::CurrentFormatVersion);
        InOutHeader.Set_EngineVersion(FEngineVersion::Current().ToString());
        InOutHeader.Set_TimestampUTC(FDateTime::UtcNow());
        InOutHeader.Set_EntityCount(Entities.Num());
        InOutHeader.Set_EngineOwnedCount(EngineOwnedCount);
        InOutHeader.Set_ConstructSpawnedCount(ConstructSpawnedCount);
        InOutHeader.Set_RuntimeSpawnedCount(RuntimeSpawnedCount);
        InOutHeader.Set_DefinitionBuiltCount(DefinitionBuiltCount);
        InOutHeader.Set_PayloadCount(Payloads.Num());
        InOutHeader.Set_UnlabeledConstructSkippedCount(UnlabeledSkipped);
        InOutHeader.Set_AnonymousSkippedCount(AnonymousSkipped);
        InOutHeader.Set_UnlabeledWithPayloadAuditCount(UnlabeledWithPayloadAudit);

        ck::snapshot::Verbose(
            TEXT("Run_CaptureV3: persisted [{}] entities (EngineOwned [{}], ConstructSpawned [{}], RuntimeSpawned [{}], "
                 "DefinitionBuilt [{}]), [{}] payloads; skipped [{}] unlabeled ConstructSpawned + [{}] anonymous scratch (audit [{}])"),
            Entities.Num(), EngineOwnedCount, ConstructSpawnedCount, RuntimeSpawnedCount, DefinitionBuiltCount, Payloads.Num(),
            UnlabeledSkipped, AnonymousSkipped, UnlabeledWithPayloadAudit);

        return ECk_SnapshotResult::Success;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Run_CaptureV3(
            UWorld& InWorld,
            FArchive& InByteWriter,
            FCk_Snapshot_HeaderV3& InOutHeader)
        -> ECk_SnapshotResult
    {
        auto* EcsWorld = InWorld.GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EcsWorld))
        {
            ck::snapshot::Error(TEXT("Run_CaptureV3: no UCk_EcsWorld_Subsystem_UE on World [{}]"), InWorld.GetName());
            return ECk_SnapshotResult::Failed_IO;
        }

        auto& CkRegistry = EcsWorld->Get_Registry();
        auto* RawRegistry = ck::registry_table::TryResolve(CkRegistry.Get_RegistryHandle());
        if (RawRegistry == nullptr)
        {
            ck::snapshot::Error(TEXT("Run_CaptureV3: could not resolve the raw entt registry from World [{}]"), InWorld.GetName());
            return ECk_SnapshotResult::Failed_IO;
        }

        InOutHeader.Set_WorldAssetPath(FSoftObjectPath{&InWorld});

        return Run_CaptureV3_Registry(*RawRegistry, CkRegistry.Get_RegistryHandle(), &InWorld, InByteWriter, InOutHeader);
    }
}

// --------------------------------------------------------------------------------------------------------------------
