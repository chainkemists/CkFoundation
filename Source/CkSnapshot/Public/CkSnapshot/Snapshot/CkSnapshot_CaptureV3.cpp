#include "CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h" // snapshot exclusion tags (rules 1.25 / 1.5)

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
#include "UObject/UnrealType.h" // TFieldIterator / CPF_SaveGame — bridged-actor recipe fields

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

        enum class ECk_SnapshotExclusionPolicy : uint8
        {
            None,
            SaveTransient,
            ReconstructOnly,
        };

        // A snapshot exclusion marker on a lifetime owner applies to its whole construction subtree. A child cannot
        // be restored without an intentionally omitted owner, so capturing it alone would create an orphan on load.
        // ReconstructOnly wins over SaveTransient: the enclosing feature explicitly declares that the omitted data is
        // recreated from authored/default state and therefore must not produce a data-loss audit.
        auto
            DoGet_SnapshotExclusionPolicy(
                const FCk_Handle& InHandle)
            -> ECk_SnapshotExclusionPolicy
        {
            constexpr auto MaxDepth = 256;
            auto Current = InHandle;
            auto Result = ECk_SnapshotExclusionPolicy::None;

            for (auto Depth = 0; Depth < MaxDepth; ++Depth)
            {
                if (Current.Has<ck::FTag_Snapshot_ReconstructOnly>())
                { return ECk_SnapshotExclusionPolicy::ReconstructOnly; }
                if (Current.Has<ck::FTag_Snapshot_SaveTransient>())
                { Result = ECk_SnapshotExclusionPolicy::SaveTransient; }

                if (NOT Current.Has<ck::FFragment_LifetimeOwner>())
                { break; }

                const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Current);
                if (ck::Is_NOT_Valid(Owner) || Owner == Current)
                { break; }
                Current = Owner;
            }
            return Result;
        }

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

        // Tagged-property data first (object refs by path), then the handle walker writes each FCk_Handle's raw
        // saved entity id — a handle field is Transient and would otherwise be skipped.
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

        auto
            Has_AnySaveGameProperty(
                const UClass* InClass)
            -> bool
        {
            if (InClass == nullptr)
            { return false; }

            for (TFieldIterator<FProperty> PropIt{InClass}; PropIt; ++PropIt)
            {
                if (PropIt->HasAnyPropertyFlags(CPF_SaveGame))
                { return true; }
            }
            return false;
        }

        // A bridged respawn re-creates the actor from its class path alone, so any UPROPERTY(SaveGame) it carries
        // would come back at class defaults and its BeginPlay-driven Construct would compose against nothing. Empty
        // when the class declares no SaveGame property, so the load side can treat "nothing to apply" as a plain
        // absence rather than an empty-but-present blob.
        auto
            SerializeActorSaveFields(
                const AActor* InActor)
            -> TArray<uint8>
        {
            auto Blob = TArray<uint8>{};
            if (InActor == nullptr || NOT Has_AnySaveGameProperty(InActor->GetClass()))
            { return Blob; }

            auto MemoryWriter = FMemoryWriter{Blob, /*bIsPersistent=*/true};
            constexpr auto LoadIfFindFails = true;
            auto Proxy = FObjectAndNameAsStringProxyArchive{MemoryWriter, LoadIfFindFails};
            Proxy.ArIsSaveGame = true;      // capture ONLY the CPF_SaveGame properties, symmetric with the load apply
            Proxy.SetIsPersistent(true);

            InActor->SerializeScriptProperties(Proxy);
            return Blob;
        }

        // The PlayerState unique-id; empty for standalone player 0; UNSET ⇒ not a player-owned entity at all.
        // ONLY the possessed pawn carries player-rendezvous identity. Controller/PlayerState entities are
        // engine-recreated infra: a saved row for them cross-maps onto the PAWN's entity at load
        // (DoResolve_PlayerEntity resolves via State->GetPawn()) and steals the keyed pawn row's adoption —
        // the DuplicateSaveKey pawn-skip incident, 2026-07-28.
        auto
            TryResolve_PlayerRendezvous(
                const FCk_Handle& InHandle)
            -> TOptional<FString>
        {
            const auto* Actor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(InHandle);
            if (Actor == nullptr)
            { return {}; }

            const auto* AsPawn = Cast<APawn>(Actor);
            if (AsPawn == nullptr || AsPawn->IsPlayerControlled() == false)
            { return {}; }

            const auto* PlayerState = AsPawn->GetPlayerState();
            if (PlayerState == nullptr)
            { return FString{}; }

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
        // Truly empty entities never appear — they are anonymous scratch (rule 5) and would be skipped anyway.
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

        const auto FindFirstProducingType = [&](FCk_Handle& InEntity, FString* OutPayloadDetail = nullptr) -> const UScriptStruct*
        {
            for (const auto* Type : SaveTypes)
            {
                const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Type);
                if (Handler == nullptr || NOT Handler->Produce)
                { continue; }
                const auto Payload = Handler->Produce(InEntity);
                if (NOT Payload.IsSet())
                { continue; }
                if (OutPayloadDetail != nullptr && Payload->GetScriptStruct() != nullptr)
                {
                    Payload->GetScriptStruct()->ExportText(
                        *OutPayloadDetail, Payload->GetMemory(), nullptr, nullptr, PPF_None, nullptr);
                }
                return Type;
            }
            return nullptr;
        };

        auto Classified = TArray<FClassified>{};
        auto PersistedIds = TSet<uint32>{};
        auto AnonymousSkipped = 0;
        auto UnlabeledSkipped = 0;
        auto UnlabeledWithPayloadAudit = 0;
        auto SaveTransientSkipped = 0;

        for (const auto RawId : CandidateIds)
        {
            if (TransientId.IsSet() && RawId == TransientId.GetValue())
            { continue; }

            auto Handle = ck::MakeHandle(FCk_Entity{static_cast<ck::SnapshotEntityType>(RawId)}, CkRegistry);
            if (ck::Is_NOT_Valid(Handle))
            { continue; }

            if (IsMarkedForDestruction(Handle))
            { continue; }

            const auto ExclusionPolicy = DoGet_SnapshotExclusionPolicy(Handle);

            // Reconstruct-only: an explicit feature policy says this entity (or its reconstruction-owned ancestor) is
            // rebuilt from authored defaults after load, so its payload omission is intentional and must not raise the
            // audit below.
            if (ExclusionPolicy == ECk_SnapshotExclusionPolicy::ReconstructOnly)
            { continue; }

            if (ExclusionPolicy == ECk_SnapshotExclusionPolicy::SaveTransient)
            {
                ++SaveTransientSkipped;
                if (const auto* ProducingType = FindFirstProducingType(Handle))
                {
                    ck::snapshot::Warning(
                        TEXT("v3 capture AUDIT: save-transient entity [{}] carries a hydration payload that will be "
                             "DROPPED (first producer [{}]) — either stop stamping FTag_Snapshot_SaveTransient on it or move the payload "
                             "to its persisted owner."),
                        Handle, GetNameSafe(ProducingType));
                }
                continue;
            }

            auto Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
            auto bPersist = false;

            // ActorSpawnIntent is the opt-in saying the stored actor recipe must rebuild this entity, so a bridged
            // actor is loader-owned even when keyed; SaveKey stays orthogonal identity, republished after rebuild.
            if (Handle.Has<ck::FFragment_SpawnRecipe>() && Handle.Has<FFragment_ActorSpawnIntent>())
            {
                Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
                bPersist = true;
            }
            else if (Handle.Has<FFragment_SaveKey>() ||
                (InWorldOrNull != nullptr && TryResolve_PlayerRendezvous(Handle).IsSet()))
            {
                Provenance = ECk_Snapshot_V3_Provenance::EngineOwned;
                bPersist = true;
            }
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
                    auto PayloadDetail = FString{};
                    if (const auto* ProducingType = FindFirstProducingType(Handle, &PayloadDetail))
                    {
                        ++UnlabeledWithPayloadAudit;
                        // Guarded: a registry-rooted entity legitimately has no lifetime owner (Get_LifetimeOwner ensures).
                        const auto OwnerId = Handle.Has<ck::FFragment_LifetimeOwner>()
                            ? Get_SavedId(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Handle))
                            : k_NoEntity;
                        ck::snapshot::Warning(
                            TEXT("v3 capture AUDIT: unlabeled ConstructSpawned child [{}] (owner saved-id [{}]) carries a "
                                 "hydration payload that will be DROPPED (first producer [{}]) — it is save-transient. Give the child a "
                                  "GameplayLabel under its owner to persist it. Payload detail: [{}]"),
                            Handle, OwnerId, GetNameSafe(ProducingType), PayloadDetail);
                    }
                }
            }
            else if (Handle.Has<ck::FFragment_SpawnRecipe>())
            {
                Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;
                bPersist = true;
            }
            else if (Handle.Has<ck::FFragment_BuildRecipe>())
            {
                Provenance = ECk_Snapshot_V3_Provenance::DefinitionBuilt;
                bPersist = true;
            }
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
                    if (Handle.Has<FFragment_SaveKey>())
                    { Entry.Set_SaveKey(Handle.Get<FFragment_SaveKey>().Get_Key()); }

                    const auto& Recipe = Handle.Get<ck::FFragment_SpawnRecipe>();
                    if (const auto ScriptClass = Recipe.Get_ScriptClass(); ck::IsValid(ScriptClass.Get()))
                    { Entry.Set_ScriptClassPath(ScriptClass->GetPathName()); }

                    // Flagged, then written anyway — a dangling ref is still better than a missing recipe.
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
                    {
                        Entry.Set_ActorClassPath(Handle.Get<FFragment_ActorSpawnIntent>().Get_ActorClassPath());
                        Entry.Set_ActorSaveFieldBytes(
                            SerializeActorSaveFields(UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(Handle)));
                    }

                    if (UCk_Utils_Transform_UE::Has(Handle))
                    { Entry.Set_ActorSpawnTransform(UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(Handle)); }
                    break;
                }
                case ECk_Snapshot_V3_Provenance::DefinitionBuilt:
                {
                    ++DefinitionBuiltCount;

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
                 "DefinitionBuilt [{}]), [{}] payloads; skipped [{}] unlabeled ConstructSpawned + [{}] anonymous scratch "
                 "+ [{}] save-transient (audit [{}])"),
            Entities.Num(), EngineOwnedCount, ConstructSpawnedCount, RuntimeSpawnedCount, DefinitionBuiltCount, Payloads.Num(),
            UnlabeledSkipped, AnonymousSkipped, SaveTransientSkipped, UnlabeledWithPayloadAudit);

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
