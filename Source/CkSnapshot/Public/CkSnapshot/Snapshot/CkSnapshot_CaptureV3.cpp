#include "CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/Settings/CkSnapshot_Settings.h"

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

#include "CkCore/Format/CkFormat.h" // ck::Format_UE — audit example rendering

#include "CkLabel/CkLabel_Utils.h"

#include "CkEcsExt/OwningActor/CkActorSpawnIntent_Fragment.h" // FFragment_ActorSpawnIntent
#include "CkEcsExt/Transform/CkTransform_Utils.h"             // bridged-actor spawn transform

#include "Misc/EngineVersion.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
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
        struct FAncestry
        {
            ECk_SnapshotExclusionPolicy _Policy = ECk_SnapshotExclusionPolicy::None;
            int32                       _Depth  = 0;
        };

        // One walk for both answers: an excluded entity is never persisted, so its depth is never read and the
        // ReconstructOnly early-out costs nothing.
        auto
            DoScan_Ancestry(
                const FCk_Handle& InHandle)
            -> FAncestry
        {
            constexpr auto MaxDepth = 256;
            auto Current = InHandle;
            auto Result  = FAncestry{};

            while (Result._Depth < MaxDepth)
            {
                if (Current.Has<ck::FTag_Snapshot_ReconstructOnly>())
                { return FAncestry{ECk_SnapshotExclusionPolicy::ReconstructOnly, Result._Depth}; }
                if (Current.Has<ck::FTag_Snapshot_SaveTransient>())
                { Result._Policy = ECk_SnapshotExclusionPolicy::SaveTransient; }

                if (NOT Current.Has<ck::FFragment_LifetimeOwner>())
                { break; }

                const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Current);
                if (ck::Is_NOT_Valid(Owner) || Owner == Current)
                { break; }

                Current = Owner;
                ++Result._Depth;
            }
            return Result;
        }

        // Tagged-property data first (object refs by path), then the handle walker writes each FCk_Handle's raw
        // saved entity id — a handle field is Transient and would otherwise be skipped.
        // RemapHandles writes each handle back — with an identical value on save — so it needs mutable memory
        // even though the struct's content never changes. Callers that already OWN the struct pass it here.
        auto
            Serialize_OwnedStruct(
                FInstancedStruct& InOutStruct)
            -> TArray<uint8>
        {
            auto Blob = TArray<uint8>{};
            if (InOutStruct.GetScriptStruct() == nullptr)
            { return Blob; }

            auto MemoryWriter = FMemoryWriter{Blob, /*bIsPersistent=*/true};
            constexpr auto LoadIfFindFails = true;
            auto Proxy = FObjectAndNameAsStringProxyArchive{MemoryWriter, LoadIfFindFails};
            Proxy.ArIsSaveGame = false;      // false ⇒ capture every non-Transient UPROPERTY, regardless of CPF_SaveGame
            Proxy.SetIsPersistent(true);

            auto Context = ck::FSnapshotContext{}; // save-mode handle write (raw id); no loader remap on save

            InOutStruct.Serialize(Proxy);
            ck::snapshot::RemapHandles(InOutStruct.GetScriptStruct(), InOutStruct.GetMutableMemory(), Proxy, Context);
            return Blob;
        }

        // For callers reading LIVE ecs memory, which must not be written to at all.
        auto
            SerializeInstancedStruct(
                const FInstancedStruct& InStruct)
            -> TArray<uint8>
        {
            auto Copy = FInstancedStruct{InStruct};
            return Serialize_OwnedStruct(Copy);
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
            FCk_Snapshot_HeaderV3& InOutHeader,
            FCaptureTimings* OutTimings)
        -> ECk_SnapshotResult
    {
        using namespace ck_snapshot_capturev3;

        TRACE_CPUPROFILER_EVENT_SCOPE(ck_snapshot_Run_CaptureV3_Registry);

        auto CkRegistry = FCk_Registry{InRegistryHandle};

        const auto* Settings = GetDefault<UCk_Snapshot_Settings>();
        const auto AuditMode = Settings != nullptr
            ? Settings->Get_CaptureAuditMode()
            : ECk_Snapshot_CaptureAuditMode::Summary;
        const auto AuditMaxExamples = Settings != nullptr ? Settings->Get_CaptureAudit_MaxExamples() : 5;
        const auto AuditDetailed = AuditMode == ECk_Snapshot_CaptureAuditMode::Detailed;

        auto Timings = FCaptureTimings{};
        auto ClassifyStopwatch = TOptional<FCk_ScopedStopwatch>{InPlace, Timings.Classify};

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

        // Answering "did this SKIPPED entity carry a payload" costs a full Produce sweep per entity, so the
        // probe is what CaptureAuditMode gates — not the logging.
        const auto FindFirstProducingType = [&](FCk_Handle& InEntity, FString* OutPayloadDetail = nullptr) -> const UScriptStruct*
        {
            if (AuditMode == ECk_Snapshot_CaptureAuditMode::Disabled)
            { return nullptr; }

            const auto AuditStopwatch = FCk_ScopedStopwatch{Timings.Audit};
            ++Timings.AuditProbeCount;

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

        auto SaveTransientWithPayloadAudit = 0;
        auto AuditExamples = TArray<FString>{};

        struct FDroppedPayload
        {
            const UScriptStruct* _ProducingType = nullptr;
            FString              _Detail;

            explicit operator bool() const { return _ProducingType != nullptr; }
        };

        // Both skip sites share the probe, the counter and the Summary example bookkeeping; only the Detailed
        // message differs, so that stays at the call site.
        const auto DoAudit_DroppedPayload = [&](FCk_Handle& InEntity, int32& InOutCount) -> FDroppedPayload
        {
            auto Result = FDroppedPayload{};
            Result._ProducingType = FindFirstProducingType(InEntity, AuditDetailed ? &Result._Detail : nullptr);

            if (NOT Result)
            { return Result; }

            ++InOutCount;
            if (NOT AuditDetailed && AuditExamples.Num() < AuditMaxExamples)
            {
                AuditExamples.Emplace(ck::Format_UE(TEXT("[{}] (producer [{}])"),
                    InEntity, GetNameSafe(Result._ProducingType)));
            }
            return Result;
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

            const auto Ancestry = DoScan_Ancestry(Handle);
            const auto ExclusionPolicy = Ancestry._Policy;

            // Reconstruct-only: an explicit feature policy says this entity (or its reconstruction-owned ancestor) is
            // rebuilt from authored defaults after load, so its payload omission is intentional and must not raise the
            // audit below.
            if (ExclusionPolicy == ECk_SnapshotExclusionPolicy::ReconstructOnly)
            { continue; }

            if (ExclusionPolicy == ECk_SnapshotExclusionPolicy::SaveTransient)
            {
                ++SaveTransientSkipped;
                if (const auto Audit = DoAudit_DroppedPayload(Handle, SaveTransientWithPayloadAudit);
                    Audit && AuditDetailed)
                {
                    ck::snapshot::Warning(
                        TEXT("v3 capture AUDIT: save-transient entity [{}] carries a hydration payload that will be "
                             "DROPPED (first producer [{}]) — either stop stamping FTag_Snapshot_SaveTransient on it or move the payload "
                             "to its persisted owner."),
                        Handle, GetNameSafe(Audit._ProducingType));
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
                    if (const auto Audit = DoAudit_DroppedPayload(Handle, UnlabeledWithPayloadAudit);
                        Audit && AuditDetailed)
                    {
                        // Guarded: a registry-rooted entity legitimately has no lifetime owner (Get_LifetimeOwner ensures).
                        const auto OwnerId = Handle.Has<ck::FFragment_LifetimeOwner>()
                            ? Get_SavedId(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Handle))
                            : k_NoEntity;
                        ck::snapshot::Warning(
                            TEXT("v3 capture AUDIT: unlabeled ConstructSpawned child [{}] (owner saved-id [{}]) carries a "
                                 "hydration payload that will be DROPPED (first producer [{}]) — it is save-transient. Give the child a "
                                  "GameplayLabel under its owner to persist it. Payload detail: [{}]"),
                            Handle, OwnerId, GetNameSafe(Audit._ProducingType), Audit._Detail);
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
            Classified.Add(FClassified{Handle, RawId, Provenance, Ancestry._Depth});
        }

        // ---- Order owners before dependents (lifetime-topology; ties by saved-id for determinism) ------------------
        Classified.Sort([](const FClassified& InA, const FClassified& InB) -> bool
        {
            return InA._Depth != InB._Depth ? InA._Depth < InB._Depth : InA._SavedId < InB._SavedId;
        });

        ClassifyStopwatch.Reset();
        auto PayloadStopwatch = TOptional<FCk_ScopedStopwatch>{InPlace, Timings.Payloads};

        auto DistinctTypePaths = TSet<const UScriptStruct*>{};

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
                            ck::IsValid(Archetype))
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

                auto Produced = TOptional<FInstancedStruct>{};
                {
                    const auto ProduceStopwatch = FCk_ScopedStopwatch{Timings.PayloadsProduce};
                    Produced = Handler->Produce(Handle);
                }
                if (NOT Produced.IsSet())
                { continue; }

                auto PayloadEntry = FCk_Snapshot_V3_PayloadEntry{};
                PayloadEntry.Set_OwnerSavedId(Item._SavedId);
                PayloadEntry.Set_TypePath(Type->GetPathName());
                {
                    const auto SerializeStopwatch = FCk_ScopedStopwatch{Timings.PayloadsSerialize};
                    PayloadEntry.Set_PayloadBytes(Serialize_OwnedStruct(Produced.GetValue()));
                }
                Timings.PayloadByteTotal += PayloadEntry.Get_PayloadBytes().Num();
                DistinctTypePaths.Add(Type);
                Payloads.Emplace(MoveTemp(PayloadEntry));
            }
        }

        PayloadStopwatch.Reset();

        // ---- Serialize the tables + stamp the header census --------------------------------------------------------
        {
            const auto TableStopwatch = FCk_ScopedStopwatch{Timings.Tables};
            FCk_Snapshot_V3_Tables::StaticStruct()->SerializeItem(InByteWriter, &Tables, /*Defaults=*/nullptr);
        }

        Timings.DistinctTypePaths = DistinctTypePaths.Num();
        if (OutTimings != nullptr)
        { *OutTimings = Timings; }

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
        InOutHeader.Set_UnlabeledWithPayloadAuditCount(
            AuditMode == ECk_Snapshot_CaptureAuditMode::Disabled
                ? FCk_Snapshot_HeaderV3::k_AuditNotMeasured
                : UnlabeledWithPayloadAudit);

        ck::snapshot::Verbose(
            TEXT("Run_CaptureV3: persisted [{}] entities (EngineOwned [{}], ConstructSpawned [{}], RuntimeSpawned [{}], "
                 "DefinitionBuilt [{}]), [{}] payloads; skipped [{}] unlabeled ConstructSpawned + [{}] anonymous scratch "
                 "+ [{}] save-transient (audit [{}])"),
            Entities.Num(), EngineOwnedCount, ConstructSpawnedCount, RuntimeSpawnedCount, DefinitionBuiltCount, Payloads.Num(),
            UnlabeledSkipped, AnonymousSkipped, SaveTransientSkipped, UnlabeledWithPayloadAudit);

        // Summary's single aggregated report. Same data-loss FACT the per-entity storm carried, minus the
        // per-entity ExportText; Detailed already logged each one individually above.
        if (const auto DroppedPayloadCount = UnlabeledWithPayloadAudit + SaveTransientWithPayloadAudit;
            DroppedPayloadCount > 0 && NOT AuditDetailed)
        {
            ck::snapshot::Warning(
                TEXT("v3 capture AUDIT: [{}] skipped entities carry a hydration payload that will be DROPPED "
                     "([{}] unlabeled ConstructSpawned + [{}] save-transient). Give an unlabeled child a GameplayLabel "
                     "under its owner to persist it, or move the payload to its persisted owner. Examples: {}. "
                     "Set CkSnapshot's CaptureAuditMode to Detailed for the per-entity payload dump."),
                DroppedPayloadCount, UnlabeledWithPayloadAudit, SaveTransientWithPayloadAudit,
                FString::Join(AuditExamples, TEXT(", ")));
        }

        return ECk_SnapshotResult::Success;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Run_CaptureV3(
            UWorld& InWorld,
            FArchive& InByteWriter,
            FCk_Snapshot_HeaderV3& InOutHeader,
            FCaptureTimings* OutTimings)
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

        return Run_CaptureV3_Registry(*RawRegistry, CkRegistry.Get_RegistryHandle(), &InWorld, InByteWriter, InOutHeader, OutTimings);
    }
}

// --------------------------------------------------------------------------------------------------------------------
