#include "CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h"

#include "CkSnapshot/CkSnapshot_Log.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/Settings/CkSnapshot_Settings.h"
#include "CkSnapshot/Snapshot/CkSnapshot_RuntimeSpawnPolicy.h"

#include "CkCore/Format/CkFormat.h" // ck::Format_UE — naming an entity the capture did not carry

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

#include "Algo/Accumulate.h"
#include "Async/ParallelFor.h"
#include "Containers/BitArray.h"
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
            if (ck::Is_NOT_Valid(InOutStruct.GetScriptStruct()))
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
            if (ck::Is_NOT_Valid(InClass))
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
            if (ck::Is_NOT_Valid(InActor) || NOT Has_AnySaveGameProperty(InActor->GetClass()))
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
            if (ck::Is_NOT_Valid(Actor))
            { return {}; }

            const auto* AsPawn = Cast<APawn>(Actor);
            if (ck::Is_NOT_Valid(AsPawn) || NOT AsPawn->IsPlayerControlled())
            { return {}; }

            const auto* PlayerState = AsPawn->GetPlayerState();
            if (ck::Is_NOT_Valid(PlayerState))
            { return FString{}; }

            const auto UniqueId = PlayerState->GetUniqueId();
            return UniqueId.IsValid() ? UniqueId.ToString() : FString{};
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace ck_snapshot_capturev3_audit
    {
        auto
            Find_RuntimeSpawnedRebuildBlocker(
                const FCk_Handle&    InTarget,
                const TSet<uint32>&  InPersistedIds)
            -> FCk_Handle
        {
            constexpr auto MaxOwnerDepth = 256;
            auto Current = InTarget;

            for (auto Depth = 0; Depth < MaxOwnerDepth; ++Depth)
            {
                const auto CurrentIsNonBridgedRuntimeSpawned =
                    Current.Has<ck::FFragment_SpawnRecipe>() &&
                    NOT Current.Has<FFragment_ActorSpawnIntent>();
                const auto CurrentDependsOnLifetimeOwner =
                    CurrentIsNonBridgedRuntimeSpawned || Current.Has<ck::FTag_ConstructSpawned>();
                if (NOT CurrentDependsOnLifetimeOwner)
                { return {}; }

                if (NOT Current.Has<ck::FFragment_LifetimeOwner>())
                { return {}; }

                const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Current);
                if (ck::Is_NOT_Valid(Owner) || Owner == Current)
                { return {}; }

                if (CurrentIsNonBridgedRuntimeSpawned)
                {
                    const auto& Recipe = Current.Get<ck::FFragment_SpawnRecipe>();
                    const auto ScriptClass = Recipe.Get_ScriptClass();
                    const auto* ScriptDefault = ck::IsValid(ScriptClass.Get())
                        ? ScriptClass->GetDefaultObject<UCk_EntityScript_UE>()
                        : nullptr;
                    if (ck::IsValid(ScriptDefault))
                    {
                        const auto OwnerSavedId = static_cast<uint32>(Owner.Get_Entity().Get_ID());
                        const auto CanRebuildCurrent =
                            ck::snapshot::runtime_spawn_policy::CanRebuildRuntimeSpawnedWithOwnerPolicy(
                                ScriptDefault->Get_IsSnapshotRespawnable(), OwnerSavedId, InPersistedIds);
                        if (NOT CanRebuildCurrent)
                        { return Current; }
                    }
                }

                const auto OwnerSavedId = static_cast<uint32>(Owner.Get_Entity().Get_ID());
                if (NOT InPersistedIds.Contains(OwnerSavedId))
                { return {}; }

                Current = Owner;
            }

            return {};
        }

        // ------------------------------------------------------------------------------------------------------

        // A durable value that names an entity the save does not write, or writes but cannot rebuild, comes back as
        // a tombstone: the feature is structurally present and functionally dead. Nothing earlier can catch it —
        // persistence and reconstruction are properties of that ENTITY at this instant, not of the field's type —
        // so capture is the first and last point at which the fact exists. It is reported here rather than dropped
        // quietly, and the save still proceeds: a world that loads with one named gap beats a save nobody can write.
        auto Audit_DurableHandles(
            const UScriptStruct*        InPayloadType,
            void*                       InPayloadMemory,
            const FCk_Handle&           InOwner,
            uint32                      InOwnerSavedId,
            const TSet<uint32>&         InPersistedIds,
            FCk_Snapshot_SaveReport&    OutReport) -> void
        {
            if (InPayloadType == nullptr || InPayloadMemory == nullptr)
            { return; }

            ck::snapshot::ForEachDurableHandle(InPayloadType, InPayloadMemory,
                [&](FCk_Handle& InTarget, const FString& InFieldPath) -> void
                {
                    if (ck::Is_NOT_Valid(InTarget))
                    { return; }

                    const auto TargetId = static_cast<uint32>(InTarget.Get_Entity().Get_ID());
                    const auto TargetIdentity = UCk_Utils_GameplayLabel_UE::Has(InTarget)
                        ? UCk_Utils_GameplayLabel_UE::Get_Label(InTarget).ToString()
                        : ck::Format_UE(TEXT("{}"), InTarget);
                    if (InPersistedIds.Contains(TargetId))
                    {
                        // The existing target is in the table, but it can still depend on a row the loader skips.
                        // Walk that saved ownership chain while the live recipes are available.
                        const auto RebuildBlocker = Find_RuntimeSpawnedRebuildBlocker(InTarget, InPersistedIds);
                        if (ck::Is_NOT_Valid(RebuildBlocker))
                        { return; }

                        const auto RebuildBlockerOwner =
                            UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(RebuildBlocker);
                        const auto& RebuildBlockerRecipe = RebuildBlocker.Get<ck::FFragment_SpawnRecipe>();
                        const auto RebuildBlockerScriptClass = RebuildBlockerRecipe.Get_ScriptClass();

                        CK_ENSURE_IF_NOT(false,
                            TEXT("v3 capture: lasting payload [{}] on source entity [{}] holds a handle at [{}] to "
                                 "target entity [{}]. The target is saved, but its owner chain includes "
                                 "runtime-created entity [{}] with temporary owner [{}] that is not saved. Its entity "
                                 "script [{}] is not marked to return after loading, so the target will not return and "
                                 "this lasting reference will be empty. Mark the named blocked entity "
                                 "snapshot-respawnable, persist its owner, or make this reference session-only."),
                            InPayloadType, InOwner, InFieldPath, InTarget, RebuildBlocker, RebuildBlockerOwner,
                            RebuildBlockerScriptClass.Get()) {}

                        auto Loss = FCk_Snapshot_SaveLossRecord{};
                        Loss.Set_PayloadType(InPayloadType->GetPathName());
                        Loss.Set_FieldPath(InFieldPath);
                        Loss.Set_OwnerSavedId(InOwnerSavedId);
                        Loss.Set_TargetEntityId(TargetId);
                        Loss.Set_TargetIdentity(TargetIdentity);
                        Loss.Set_Reason(
                            TEXT("runtime owner-chain entity is not snapshot-respawnable and cannot reconstruct"));
                        OutReport.Add_Loss(MoveTemp(Loss));
                        return;
                    }

                    CK_ENSURE_IF_NOT(false,
                        TEXT("v3 capture: durable payload [{}] on entity [{}] holds a handle at [{}] to entity [{}], "
                            "which this save does NOT persist. It will load back as a tombstone — the feature comes "
                            "back structurally complete and functionally dead. Either persist the target, or make "
                            "the field session state the owner's setup re-derives."),
                        InPayloadType, InOwner, InFieldPath, InTarget) {}

                    auto Loss = FCk_Snapshot_SaveLossRecord{};
                    Loss.Set_PayloadType(InPayloadType->GetPathName());
                    Loss.Set_FieldPath(InFieldPath);
                    Loss.Set_OwnerSavedId(InOwnerSavedId);
                    Loss.Set_TargetEntityId(TargetId);
                    Loss.Set_TargetIdentity(TargetIdentity);
                    Loss.Set_Reason(TEXT("handle to a non-persisted entity"));

                    OutReport.Add_Loss(MoveTemp(Loss));
                });
        }

        // ------------------------------------------------------------------------------------------------------

        // The object-reference sibling of Audit_DurableHandles. Same question, other kind of reference: a durable
        // payload may name an OBJECT no load will bring back. The reference that motivates it is the runtime-built
        // material instance -- created per session, stored into a durable field as a soft path, and on load that
        // path names nothing, so the feature comes back with a reference it cannot resolve.
        //
        // The discriminator runs HERE, at capture, because this is the only moment the object still exists and can
        // be asked what it is. At load all that survives is a path that failed, where "was never an asset" and
        // "asset was deleted" are indistinguishable.
        //
        // REPORT-ONLY for now, deliberately, and NOT an ensure like its handle sibling: the predicate below is a
        // first approximation over a population nobody has enumerated yet, so its first job is to produce a census
        // to judge, not to halt a save on a rule that has never been tested against real content. Promote it to an
        // ensure once the census is clean and the predicate has been shown to have no false positives.
        auto Audit_DurableObjectRefs(
            const UScriptStruct*        InPayloadType,
            void*                       InPayloadMemory,
            const FCk_Handle&           InOwner,
            uint32                      InOwnerSavedId,
            FCk_Snapshot_SaveReport&    OutReport) -> void
        {
            if (InPayloadType == nullptr || InPayloadMemory == nullptr)
            { return; }

            ck::snapshot::ForEachDurableObjectRef(InPayloadType, InPayloadMemory,
                [&](const UObject& InObject, const FString& InFieldPath) -> void
                {
                    // An ASSET has a package on disk, so its path resolves in any future session -- that is the
                    // whole of what makes a reference persistable.
                    if (InObject.IsAsset())
                    { return; }

                    // Not an asset, but not necessarily unsaveable: a level actor or component is not an asset and
                    // its path DOES name something a loaded level rebuilds. RF_Transient is the author of the
                    // object saying it belongs to this session only, which is what separates the runtime-built
                    // material instance from the placed actor.
                    if (NOT InObject.HasAnyFlags(RF_Transient))
                    { return; }

                    auto Loss = FCk_Snapshot_SaveLossRecord{};
                    Loss.Set_PayloadType(InPayloadType->GetPathName());
                    Loss.Set_FieldPath(InFieldPath);
                    Loss.Set_OwnerSavedId(InOwnerSavedId);
                    Loss.Set_TargetIdentity(InObject.GetPathName());
                    Loss.Set_Reason(TEXT("transient object reference in a durable payload"));

                    OutReport.Add_Loss(MoveTemp(Loss));
                });
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
            FCk_Snapshot_SaveReport& OutReport,
            FCaptureTimings* OutTimings)
        -> ECk_SnapshotResult
    {
        using namespace ck_snapshot_capturev3;

        TRACE_CPUPROFILER_EVENT_SCOPE(ck_snapshot_Run_CaptureV3_Registry);

        auto CkRegistry = FCk_Registry{InRegistryHandle};

        const auto* Settings = GetDefault<UCk_Snapshot_Settings>();
        const auto SettingsAreValid = ck::IsValid(Settings);
        const auto AuditMode = SettingsAreValid
            ? Settings->Get_CaptureAuditMode()
            : ECk_Snapshot_CaptureAuditMode::Summary;
        const auto AuditMaxExamples = SettingsAreValid ? Settings->Get_CaptureAudit_MaxExamples() : 5;
        const auto AuditDetailed = AuditMode == ECk_Snapshot_CaptureAuditMode::Detailed;
        const auto ParallelSerializeFlags = NOT SettingsAreValid ||
            Settings->Get_ParallelPayloadSerialization() == ECk_EnableDisable::Enable
                ? EParallelForFlags::None
                : EParallelForFlags::ForceSingleThread;

        auto Timings = FCaptureTimings{};
        auto ClassifyStopwatch = TOptional<FCk_ScopedStopwatch>{InPlace, Timings.Classify};

        // ---- Collect candidate entities (every entity carrying ≥1 fragment/tag) -----------------------------------
        // Truly empty entities never appear — they are anonymous scratch (rule 5) and would be skipped anyway.
        // Dedup is a bit per entity index rather than a hashed set: this pass touches every fragment instance
        // in the registry (~15x the entity count), so per-touch cost dominates.
        const auto EntityStorageHash = static_cast<uint32>(entt::type_hash<ck::SnapshotEntityType>::value());
        auto CandidateIds = TArray<uint32>{};
        auto SeenEntityIndices = TBitArray<>{};
        for (auto&& StoragePair : InRegistry.storage())
        {
            const auto TypeHash = static_cast<uint32>(StoragePair.second.info().hash());
            if (TypeHash == EntityStorageHash)
            { continue; }

            for (const auto Entity : StoragePair.second)
            {
                if (Entity == entt::tombstone)
                { continue; }

                const auto EntityIndex = static_cast<int32>(entt::to_entity(Entity));
                SeenEntityIndices.PadToNum(EntityIndex + 1, false);
                if (SeenEntityIndices[EntityIndex])
                { continue; }

                SeenEntityIndices[EntityIndex] = true;
                CandidateIds.Add(static_cast<uint32>(Entity));
            }
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
                if (ck::Is_NOT_Valid(Handler, ck::IsValid_Policy_NullptrOnly{}) || NOT Handler->Produce)
                { continue; }
                const auto Payload = Handler->Produce(InEntity);
                if (NOT Payload.IsSet())
                { continue; }
                if (ck::IsValid(OutPayloadDetail, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(Payload->GetScriptStruct()))
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

            explicit operator bool() const { return ck::IsValid(_ProducingType); }
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

        auto UncapturedRuntimeWithPayload = 0;

        for (const auto RawId : CandidateIds)
        {
            auto Handle = ck::MakeHandle(FCk_Entity{static_cast<ck::SnapshotEntityType>(RawId)}, CkRegistry);
            if (ck::Is_NOT_Valid(Handle))
            { continue; }

            if (TransientId.IsSet() && RawId == TransientId.GetValue())
            {
                // The world transient is bookkeeping, never world state, so it is never persisted — but a handler
                // that produces FOR it is a declaration defect rather than a design choice: durable state was put
                // somewhere the save cannot reach, and no ratchet can see it, because the type is fine and only
                // its OWNER is unpersistable.
                if (const auto* ProducingType = FindFirstProducingType(Handle))
                {
                    ck::snapshot::Warning(
                        TEXT("v3 capture AUDIT: the world TRANSIENT entity carries a hydration payload that will be "
                             "DROPPED (first producer [{}]). The transient is bookkeeping and is never persisted — "
                             "move that state onto an entity the save can carry."),
                        GetNameSafe(ProducingType));
                }
                continue;
            }

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
                (ck::IsValid(InWorldOrNull) && TryResolve_PlayerRendezvous(Handle).IsSet()))
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

                // Runtime-created with no recipe and no identity: nothing to rebuild it from, so it is not
                // captured. Under C5 that is normally the DESIGNED outcome — a timer an SM state started is
                // session state whose durable intent lives in the owning feature — so this is recorded as DATA
                // rather than warned about. What was wrong before is that it was invisible: an author could not
                // tell a deliberate omission from a silent drop, and the runtime-timer class went unnoticed for
                // exactly that reason. Per-item detail stays at Verbose; the save's own summary carries the count.
                if (const auto* ProducingType = FindFirstProducingType(Handle))
                {
                    ++UncapturedRuntimeWithPayload;

                    auto Record = FCk_Snapshot_UncapturedRuntimeRecord{};
                    Record.Set_Identity(ck::Format_UE(TEXT("{}"), Handle));
                    Record.Set_PayloadType(GetNameSafe(ProducingType));
                    OutReport.Add_UncapturedRuntimeEntity(MoveTemp(Record));

                    ck::snapshot::Verbose(
                        TEXT("v3 capture: runtime entity [{}] produced [{}] but is not captured (no construction "
                             "recipe and no save identity) — its state is session-scoped and the owning feature "
                             "re-creates it on load"),
                        Handle, GetNameSafe(ProducingType));
                }
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

        // Produce (game thread, reads live ECS state) is split from serialize (fork-join over the produced
        // copies) so the serialize leg can fan out. Slot i of the payload table is pending payload i, so the
        // file is byte-identical to the serial order.
        struct FPendingPayload
        {
            uint32               _OwnerSavedId = 0;
            const UScriptStruct* _Type = nullptr;
            FInstancedStruct     _Payload;
            // Carried for the census below only. A handle is a cheap value; it is stringified for at most a
            // SMALL capture, so a BB-sized world never pays for the ToString.
            FCk_Handle           _Owner;
        };
        auto PendingPayloads = TArray<FPendingPayload>{};

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

                    // Flagged, then written anyway — a dangling ref is still better than a missing recipe. The
                    // spawn recipe IS durable by construction (it is what rebuilds the entity), so it goes through
                    // the same audit as every other durable payload rather than keeping a second copy of the rule.
                    auto ParamsCopy = FInstancedStruct{Recipe.Get_SpawnParams()};
                    ck_snapshot_capturev3_audit::Audit_DurableHandles(ParamsCopy.GetScriptStruct(),
                        ParamsCopy.GetMutableMemory(), Handle, Item._SavedId, PersistedIds, OutReport);
                    ck_snapshot_capturev3_audit::Audit_DurableObjectRefs(ParamsCopy.GetScriptStruct(),
                        ParamsCopy.GetMutableMemory(), Handle, Item._SavedId, OutReport);
                    // Assigned through the mutable getter: the generated Set_ takes const& and would copy the blob.
                    Entry.Get_SpawnParamsBytes() = SerializeInstancedStruct(Recipe.Get_SpawnParams());

                    if (UCk_Utils_ContextOwner_UE::Has(Handle))
                    { Entry.Set_ContextOwnerSavedId(Get_SavedId(UCk_Utils_ContextOwner_UE::Get_ContextOwner(Handle))); }

                    if (Handle.Has<FFragment_ActorSpawnIntent>())
                    {
                        Entry.Set_ActorClassPath(Handle.Get<FFragment_ActorSpawnIntent>().Get_ActorClassPath());
                        Entry.Get_ActorSaveFieldBytes() =
                            SerializeActorSaveFields(UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor(Handle));
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
                if (ck::Is_NOT_Valid(Handler, ck::IsValid_Policy_NullptrOnly{}) || NOT Handler->Produce)
                { continue; }

                auto Produced = TOptional<FInstancedStruct>{};
                {
                    const auto ProduceStopwatch = FCk_ScopedStopwatch{Timings.PayloadsProduce};
                    Produced = Handler->Produce(Handle);
                }
                if (NOT Produced.IsSet())
                { continue; }

                // Audited in place rather than on a copy: Audit_DurableHandles only reads, and this loop runs per
                // payload per entity — the copy it used to take is exactly the kind this path was cleaned of.
                ck_snapshot_capturev3_audit::Audit_DurableHandles(Produced.GetValue().GetScriptStruct(),
                    Produced.GetValue().GetMutableMemory(), Handle, Item._SavedId, PersistedIds, OutReport);
                ck_snapshot_capturev3_audit::Audit_DurableObjectRefs(Produced.GetValue().GetScriptStruct(),
                    Produced.GetValue().GetMutableMemory(), Handle, Item._SavedId, OutReport);

                DistinctTypePaths.Add(Type);
                PendingPayloads.Emplace(FPendingPayload{Item._SavedId, Type, MoveTemp(Produced.GetValue()), Handle});
            }
        }

        // ---- Payload census: WHAT was captured, by type ------------------------------------------------------------
        // The report's buckets partition payloads by OUTCOME, never by type, so they cannot answer "why does a save
        // taken after a load carry one more payload than the save that built that world". This names the producers.
        // Bounded by the DISTINCT-type count rather than the payload count, so a BB-sized world still logs one line.
        {
            auto CensusByType = TMap<FString, int32>{};
            for (const auto& Pending : PendingPayloads)
            { CensusByType.FindOrAdd(GetNameSafe(Pending._Type)) += 1; }

            auto CensusKeys = TArray<FString>{};
            CensusByType.GetKeys(CensusKeys);
            CensusKeys.Sort();

            auto CensusParts = TArray<FString>{};
            CensusParts.Reserve(CensusKeys.Num());
            for (const auto& Key : CensusKeys)
            { CensusParts.Emplace(FString::Printf(TEXT("%s x%d"), *Key, CensusByType[Key])); }

            ck::snapshot::Display(TEXT("v3 capture CENSUS: [{}] payloads over [{}] type(s) - {}"),
                PendingPayloads.Num(), CensusKeys.Num(), FString::Join(CensusParts, TEXT(", ")));

            // A per-type count says WHAT grew; it cannot say WHICH entity grew it, and for the idempotency
            // question ("a save taken after a load carries one more payload than the save that built the world")
            // the owner is the whole answer. Small captures only: gate is on the payload count so a BB world
            // never stringifies thousands of handles.
            static constexpr auto k_CensusOwnerDetailMaxPayloads = 32;
            if (PendingPayloads.Num() <= k_CensusOwnerDetailMaxPayloads)
            {
                for (auto& Pending : PendingPayloads)
                {
                    ck::snapshot::Display(TEXT("v3 capture CENSUS OWNER: [{}] saved-id [{}] on [{}]"),
                        GetNameSafe(Pending._Type), Pending._OwnerSavedId, Pending._Owner);
                }
            }
        }

        // ---- Serialize the produced payloads (fork-join) -----------------------------------------------------------
        // Safe off the game thread with no GC guard: each task owns its payload copy, and the game thread is
        // captive inside the ParallelFor — GC only ever starts from the game thread, so it cannot run here.
        {
            const auto SerializeStopwatch = FCk_ScopedStopwatch{Timings.PayloadsSerialize};

            auto TypePaths = TMap<const UScriptStruct*, FString>{};
            for (const auto* Type : DistinctTypePaths)
            { TypePaths.Add(Type, Type->GetPathName()); }

            Payloads.SetNum(PendingPayloads.Num());
            ParallelFor(PendingPayloads.Num(),
                [&](int32 InIndex) -> void
                {
                    auto& Pending = PendingPayloads[InIndex];
                    auto& Entry   = Payloads[InIndex];
                    Entry.Set_OwnerSavedId(Pending._OwnerSavedId);
                    Entry.Set_TypePath(TypePaths.FindChecked(Pending._Type));
                    Entry.Get_PayloadBytes() = Serialize_OwnedStruct(Pending._Payload);
                },
                ParallelSerializeFlags);
        }

        Timings.PayloadByteTotal = Algo::TransformAccumulate(Payloads,
            [](const FCk_Snapshot_V3_PayloadEntry& InEntry) { return static_cast<int64>(InEntry.Get_PayloadBytes().Num()); },
            int64{0});

        PayloadStopwatch.Reset();

        // ---- Serialize the tables + stamp the header census --------------------------------------------------------
        {
            const auto TableStopwatch = FCk_ScopedStopwatch{Timings.Tables};
            FCk_Snapshot_V3_Tables::StaticStruct()->SerializeItem(InByteWriter, &Tables, /*Defaults=*/nullptr);
        }

        Timings.DistinctTypePaths = DistinctTypePaths.Num();
        if (ck::IsValid(OutTimings, ck::IsValid_Policy_NullptrOnly{}))
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

        // The transient-object-reference census. Audit_DurableObjectRefs only RECORDS -- unlike its handle sibling it
        // does not ensure -- so without this line its findings would sit in the report unread, which is a worse
        // failure than a log nobody reads. One aggregated line per save, naming distinct offenders: the population is
        // per-TYPE, so a world with 300 customized characters is one entry, not three hundred.
        if (const auto& AllLosses = OutReport.Get_Losses();
            AllLosses.Num() > 0)
        {
            auto ObjectRefOffenders = TSet<FString>{};
            for (const auto& Loss : AllLosses)
            {
                if (Loss.Get_Reason() != TEXT("transient object reference in a durable payload"))
                { continue; }

                ObjectRefOffenders.Add(FString::Printf(TEXT("%s.%s"), *Loss.Get_PayloadType(), *Loss.Get_FieldPath()));
            }

            if (ObjectRefOffenders.Num() > 0)
            {
                auto Sorted = ObjectRefOffenders.Array();
                Sorted.Sort();

                ck::snapshot::Warning(
                    TEXT("v3 capture AUDIT: [{}] durable field(s) hold a SESSION-ONLY object that no load will bring "
                         "back -- the field returns a path resolving to nothing and the feature comes back unable to "
                         "resolve its own reference. Either store what the value is DERIVED FROM and rebuild it at "
                         "setup, or split the field into the feature's session fragment. Offenders: {}"),
                    Sorted.Num(), FString::Join(Sorted, TEXT(", ")));
            }
        }

        // A DIFFERENT population from the audit above: those entities were skipped by an explicit rule, these were
        // eligible and simply have nothing to rebuild from. Display, not Warning, on purpose — the AngelScript
        // autotest runner escalates warnings to failures, and every test that saves would trip this census.
        // ONE line per save either way: a per-item report would drown the log.
        if (UncapturedRuntimeWithPayload > 0)
        {
            ck::snapshot::Display(
                TEXT("Run_CaptureV3: [{}] runtime-created entities produced state the save did not carry — they have "
                     "no construction recipe and no save identity, so the owning feature re-creates them on load. "
                     "Named in the save report (Get_LastSaveReport); per-entity detail at Verbose"),
                UncapturedRuntimeWithPayload);
        }

        return ECk_SnapshotResult::Success;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Run_CaptureV3(
            UWorld& InWorld,
            FArchive& InByteWriter,
            FCk_Snapshot_HeaderV3& InOutHeader,
            FCk_Snapshot_SaveReport& OutReport,
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
        if (ck::Is_NOT_Valid(RawRegistry, ck::IsValid_Policy_NullptrOnly{}))
        {
            ck::snapshot::Error(TEXT("Run_CaptureV3: could not resolve the raw entt registry from World [{}]"), InWorld.GetName());
            return ECk_SnapshotResult::Failed_IO;
        }

        InOutHeader.Set_WorldAssetPath(FSoftObjectPath{&InWorld});

        return Run_CaptureV3_Registry(*RawRegistry, CkRegistry.Get_RegistryHandle(), &InWorld, InByteWriter,
            InOutHeader, OutReport, OutTimings);
    }
}

// --------------------------------------------------------------------------------------------------------------------
