#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "UObject/SoftObjectPath.h"

#include "CkSnapshot_Header.generated.h"

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_Header_FragmentManifestEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_Header_FragmentManifestEntry);

private:
    UPROPERTY() FString  _DisplayName;
    UPROPERTY() uint32   _EnttTypeHash = 0;
    // The entt storage id this entry was captured from. For most fragments this equals _EnttTypeHash (the default
    // storage). Dynamic fragments (CkDynamic) keep one NAMED storage per AS struct type, all of value-type
    // FCk_Fragment_DynamicFragment_Data -- so several entries share _EnttTypeHash but carry distinct _StorageId.
    // Restore replays each entry onto ITS storage id via entt's id-parameterized get<T>(archive, id).
    UPROPERTY() uint32   _StorageId    = 0;
    UPROPERTY() int32    _EntityCount  = 0;
    UPROPERTY() int64    _ByteLength   = 0;
    UPROPERTY() int64    _ByteOffset   = 0;

public:
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_EnttTypeHash);
    CK_PROPERTY(_StorageId);
    CK_PROPERTY(_EntityCount);
    CK_PROPERTY(_ByteLength);
    CK_PROPERTY(_ByteOffset);

    CK_DEFINE_CONSTRUCTORS(FCk_Snapshot_Header_FragmentManifestEntry, _DisplayName, _EnttTypeHash, _EntityCount, _ByteLength, _ByteOffset);
};

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_Header
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_Header);

public:
    // Version history:
    //   1 — initial format.
    //   2 — dynamic-fragment handle remap covers TYPED handles (IsChildOf, not exact-match) + TSet/TMap containers,
    //       so v2 streams carry handle ids a v1 reader would not expect (and vice versa) — no cross-compatibility.
    static constexpr uint16 CurrentFormatVersion = 2;

private:
    UPROPERTY() uint16              _FormatVersion = CurrentFormatVersion;
    // Stored as string (FEngineVersion is not a USTRUCT and cannot be a UPROPERTY).
    // Populated via FEngineVersion::Current().ToString() on save.
    UPROPERTY() FString             _EngineVersion;
    UPROPERTY() FGuid               _PluginBuildHash;
    UPROPERTY() FDateTime           _TimestampUTC;
    UPROPERTY() FSoftObjectPath     _WorldAssetPath;
    UPROPERTY() TArray<FCk_Snapshot_Header_FragmentManifestEntry> _Manifest;
    UPROPERTY() int32               _EntityCount = 0;
    // Raw entt id (underlying uint32) of the registry's transient entity at capture time. Consumed only by the
    // live-world restore to adopt the restored transient. 0xFFFFFFFF == entt::null (registry-core captures with
    // no transient ctx leave it at the sentinel; that path never adopts).
    UPROPERTY() uint32              _TransientEntityId = 0xFFFFFFFFu;
    // Byte offset (within the snapshot stream) of the self-describing tag section, appended after all fragment
    // passes. Restore Seeks here rather than trusting the stream position, since the fragment manifest byte-jumps.
    UPROPERTY() int64               _TagSectionByteOffset = 0;

public:
    CK_PROPERTY(_FormatVersion);
    CK_PROPERTY(_EngineVersion);
    CK_PROPERTY(_PluginBuildHash);
    CK_PROPERTY(_TimestampUTC);
    CK_PROPERTY(_WorldAssetPath);
    CK_PROPERTY(_Manifest);
    CK_PROPERTY(_EntityCount);
    CK_PROPERTY(_TransientEntityId);
    CK_PROPERTY(_TagSectionByteOffset);
};

// --------------------------------------------------------------------------------------------------------------------
// Format v3 — rebuild+hydrate. v3 stores a per-entity recipe/identity table + minimal hydration payloads
// instead of a raw fragment image.

// One provenance per persisted entity. EngineOwned re-created by the level/engine flow (adopt by
// rendezvous key); ConstructSpawned re-created by its owner's replayed Construct/BeginPlay (adopt by identity);
// RuntimeSpawned re-created from a stored EntityScript recipe; DefinitionBuilt re-created from a stored
// construction-script/archetype recipe (entities built via Request_BuildAndReplicate, e.g. inventory items).
UENUM()
enum class ECk_Snapshot_V3_Provenance : uint8
{
    EngineOwned,
    ConstructSpawned,
    RuntimeSpawned,
    DefinitionBuilt,
};

// One step of a DefinitionBuilt entity's construction recipe: the construction-script class + optional archetype
// asset (both captured by path). Reconstructed on load into a FCk_EntityReplicationDriver_ConstructionInfo.
USTRUCT()
struct CKSNAPSHOT_API FCk_Snapshot_V3_BuildStep
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_V3_BuildStep);

private:
    // Paths are FStrings (not FSoftObjectPath) — the v3 tables serialize through a plain FArchive, which does not
    // support FSoftObjectPath. Reconstructed into a soft path on load. Mirrors _ScriptClassPath / _ActorClassPath.
    UPROPERTY() FString _ScriptClassPath;
    UPROPERTY() FString _ArchetypePath; // asset path; empty when the step carries no archetype

public:
    CK_PROPERTY(_ScriptClassPath);
    CK_PROPERTY(_ArchetypePath);
};

// One entry per persisted entity. Fields not relevant to the entry's provenance stay defaulted. Handle-bearing
// data (spawn params) is pre-serialized into _SpawnParamsBytes with FSnapshotContext handle-remap; the entry
// itself carries only plain fields + byte blobs, so it SerializeItem's without a handle context.
USTRUCT()
struct CKSNAPSHOT_API FCk_Snapshot_V3_EntityEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_V3_EntityEntry);

private:
    // Raw entt id (underlying uint32) of this entity in the saving world — the key every intra-save reference uses
    // (lifetime/context owner, payload owner, handles inside params). The loader builds a saved→live map.
    UPROPERTY() uint32                      _SavedId = 0xFFFFFFFFu;
    UPROPERTY() ECk_Snapshot_V3_Provenance  _Provenance = ECk_Snapshot_V3_Provenance::RuntimeSpawned;

    // Saved-id of the lifetime owner (ConstructSpawned + RuntimeSpawned). 0xFFFFFFFF == none.
    UPROPERTY() uint32                      _LifetimeOwnerSavedId = 0xFFFFFFFFu;

    // ---- EngineOwned rendezvous (exactly one is set) ----
    UPROPERTY() FGuid                       _SaveKey;    // level-actor SaveKey GUID (zero == unset)
    UPROPERTY() FString                     _PlayerId;   // PlayerState unique-id string (empty == standalone player 0)

    // ---- ConstructSpawned identity ----
    UPROPERTY() FString                     _Label;      // GameplayLabel under the owner (unique by record contract)

    // ---- RuntimeSpawned recipe ----
    UPROPERTY() FString                     _ScriptClassPath;
    UPROPERTY() TArray<uint8>               _SpawnParamsBytes;      // FInstancedStruct::Serialize + handle-remap
    UPROPERTY() uint32                      _ContextOwnerSavedId = 0xFFFFFFFFu;
    UPROPERTY() FString                     _ActorClassPath;        // FFragment_ActorSpawnIntent, if present
    // World transform captured for a bridged (_ActorClassPath set) RuntimeSpawned entity so the loader spawns the
    // actor at its saved position (the entity Transform is seeded from the actor at Construct; hydrating it would
    // be stomped by FProcessor_Transform_SyncFromActor). Identity for non-Transform entities.
    UPROPERTY() FTransform                  _ActorSpawnTransform;

    // ---- DefinitionBuilt recipe ----
    UPROPERTY() TArray<FCk_Snapshot_V3_BuildStep> _BuildRecipe;

public:
    CK_PROPERTY(_SavedId);
    CK_PROPERTY(_Provenance);
    CK_PROPERTY(_LifetimeOwnerSavedId);
    CK_PROPERTY(_SaveKey);
    CK_PROPERTY(_PlayerId);
    CK_PROPERTY(_Label);
    CK_PROPERTY(_ScriptClassPath);
    CK_PROPERTY(_SpawnParamsBytes);
    CK_PROPERTY(_ContextOwnerSavedId);
    CK_PROPERTY(_ActorClassPath);
    CK_PROPERTY(_ActorSpawnTransform);
    CK_PROPERTY(_BuildRecipe);
};

// One hydration payload: the byte image (tagged-property + handle-remap) a feature's Produce emitted for an entity.
USTRUCT()
struct CKSNAPSHOT_API FCk_Snapshot_V3_PayloadEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_V3_PayloadEntry);

private:
    UPROPERTY() uint32          _OwnerSavedId = 0xFFFFFFFFu; // the entity this payload hydrates
    UPROPERTY() FString         _TypePath;                   // the RepData UScriptStruct path
    UPROPERTY() TArray<uint8>   _PayloadBytes;               // FInstancedStruct::Serialize + handle-remap

public:
    CK_PROPERTY(_OwnerSavedId);
    CK_PROPERTY(_TypePath);
    CK_PROPERTY(_PayloadBytes);
};

// The whole v3 stream content: the entity table + the payload table. SerializeItem'd into _SnapshotBytesV3.
USTRUCT()
struct CKSNAPSHOT_API FCk_Snapshot_V3_Tables
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_V3_Tables);

private:
    UPROPERTY() TArray<FCk_Snapshot_V3_EntityEntry>  _Entities;
    UPROPERTY() TArray<FCk_Snapshot_V3_PayloadEntry> _Payloads;

public:
    CK_PROPERTY(_Entities);
    CK_PROPERTY(_Payloads);
};

// v3 header — metadata + provenance census (the inspectable counts the classification gate asserts on).
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_HeaderV3
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_HeaderV3);

public:
    static constexpr uint16 CurrentFormatVersion = 3;

private:
    UPROPERTY() uint16          _FormatVersion = CurrentFormatVersion;
    UPROPERTY() FString         _EngineVersion;
    UPROPERTY() FGuid           _PluginBuildHash;
    UPROPERTY() FDateTime       _TimestampUTC;
    UPROPERTY() FSoftObjectPath _WorldAssetPath;
    UPROPERTY() int32           _EntityCount = 0;
    UPROPERTY() int32           _EngineOwnedCount = 0;
    UPROPERTY() int32           _ConstructSpawnedCount = 0;
    UPROPERTY() int32           _RuntimeSpawnedCount = 0;
    UPROPERTY() int32           _DefinitionBuiltCount = 0;
    UPROPERTY() int32           _PayloadCount = 0;
    // Rule-3 unlabeled ConstructSpawned children (save-transient) and rule-5 anonymous scratch entities skipped
    // outright — reported in the save log line; the audit count flags unlabeled children that had a Produce payload.
    UPROPERTY() int32           _UnlabeledConstructSkippedCount = 0;
    UPROPERTY() int32           _AnonymousSkippedCount = 0;
    UPROPERTY() int32           _UnlabeledWithPayloadAuditCount = 0;

public:
    CK_PROPERTY(_FormatVersion);
    CK_PROPERTY(_EngineVersion);
    CK_PROPERTY(_PluginBuildHash);
    CK_PROPERTY(_TimestampUTC);
    CK_PROPERTY(_WorldAssetPath);
    CK_PROPERTY(_EntityCount);
    CK_PROPERTY(_EngineOwnedCount);
    CK_PROPERTY(_ConstructSpawnedCount);
    CK_PROPERTY(_RuntimeSpawnedCount);
    CK_PROPERTY(_DefinitionBuiltCount);
    CK_PROPERTY(_PayloadCount);
    CK_PROPERTY(_UnlabeledConstructSkippedCount);
    CK_PROPERTY(_AnonymousSkippedCount);
    CK_PROPERTY(_UnlabeledWithPayloadAuditCount);
};
