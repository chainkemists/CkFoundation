#pragma once

#include "CkSnapshot/Inspection/CkSnapshot_Inspection_Data.h"
#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "CkCore/Macros/CkMacros.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_SnapshotInspection_DiffGroupKind : uint8
{
    Added,
    Removed,
    CountChanged,
    PayloadsChanged,
    Unchanged,
};

// --------------------------------------------------------------------------------------------------------------------

/** Per payload TYPE, the aggregate each side carries. Appears twice: once per entity group (that group's members'
 *  payloads only) and once at diff scope (the whole save). ContentDiffers is hash-based, so a value change that
 *  keeps the byte count identical is still caught. */
class CKSNAPSHOT_API FCk_SnapshotInspection_DiffPayloadTypeRow
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_DiffPayloadTypeRow);

private:
    FString _TypePath;
    int32   _CountBaseline = 0;
    int32   _CountCurrent = 0;
    int64   _BytesBaseline = 0;
    int64   _BytesCurrent = 0;
    // True when the multiset of per-payload SHA-256 hashes differs between the sides — the only way to see a
    // same-size value change without decoding.
    bool    _ContentDiffers = false;

public:
    CK_PROPERTY(_TypePath);
    CK_PROPERTY(_CountBaseline);
    CK_PROPERTY(_CountCurrent);
    CK_PROPERTY(_BytesBaseline);
    CK_PROPERTY(_BytesCurrent);
    CK_PROPERTY(_ContentDiffers);
};

// --------------------------------------------------------------------------------------------------------------------

/** All entities on either side that share one identity path (see Diff_Documents for the path contract). Saved ids
 *  are NOT stable across saves, so the group — not the id — is the unit of comparison: identical spawns collapse
 *  into one group whose count delta IS the leak signal ("identity X: 3 → 80"). */
class CKSNAPSHOT_API FCk_SnapshotInspection_DiffEntityGroup
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_DiffEntityGroup);

private:
    FString _IdentityPath;
    // The last identity segment, humanized for a row label: the label when one exists, else the class name (last
    // dot segment), else the save key / player id, else the provenance text.
    FString _DisplayName;
    ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::EngineOwned;
    ECk_SnapshotInspection_DiffGroupKind _Kind = ECk_SnapshotInspection_DiffGroupKind::Unchanged;

    int32 _CountBaseline = 0;
    int32 _CountCurrent = 0;
    int64 _PayloadBytesBaseline = 0;
    int64 _PayloadBytesCurrent = 0;

    // Ascending, so a UI can select the group's rows in either document.
    TArray<uint32> _SavedIdsBaseline;
    TArray<uint32> _SavedIdsCurrent;

    // This group's members' payloads only, one row per type path present on either side; same ordering contract
    // as the diff-scope table.
    TArray<FCk_SnapshotInspection_DiffPayloadTypeRow> _PayloadTypes;

public:
    CK_PROPERTY(_IdentityPath);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_Provenance);
    CK_PROPERTY(_Kind);
    CK_PROPERTY(_CountBaseline);
    CK_PROPERTY(_CountCurrent);
    CK_PROPERTY(_PayloadBytesBaseline);
    CK_PROPERTY(_PayloadBytesCurrent);
    CK_PROPERTY(_SavedIdsBaseline);
    CK_PROPERTY(_SavedIdsCurrent);
    CK_PROPERTY(_PayloadTypes);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSNAPSHOT_API FCk_SnapshotInspection_DiffCensusRow
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_DiffCensusRow);

private:
    ECk_Snapshot_V3_Provenance _Provenance = ECk_Snapshot_V3_Provenance::EngineOwned;
    int32 _CountBaseline = 0;
    int32 _CountCurrent = 0;

public:
    CK_PROPERTY(_Provenance);
    CK_PROPERTY(_CountBaseline);
    CK_PROPERTY(_CountCurrent);
};

// --------------------------------------------------------------------------------------------------------------------

/** The comparison of two inspection documents. Deterministic by contract: identical input documents produce a
 *  field-identical diff, every array carries an explicitly specified order, and nothing time-, pointer- or
 *  environment-derived enters it. */
class CKSNAPSHOT_API FCk_SnapshotInspection_Diff
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_Diff);

private:
    // A diff is only meaningful between two successful reads; anything else sets Valid false and names the side
    // and status that blocked it. Every other field stays defaulted on an invalid diff.
    bool    _Valid = false;
    FString _InvalidReason;

    int32 _EntityCountBaseline = 0;
    int32 _EntityCountCurrent = 0;
    int32 _PayloadCountBaseline = 0;
    int32 _PayloadCountCurrent = 0;
    int64 _SnapshotBytesBaseline = 0;
    int64 _SnapshotBytesCurrent = 0;

    // Fixed order: EngineOwned, ConstructSpawned, RuntimeSpawned, DefinitionBuilt.
    TArray<FCk_SnapshotInspection_DiffCensusRow> _Census;

    // Sorted: |count delta| descending, then |payload-bytes delta| descending, then identity path ascending —
    // the biggest population change reads first. Unchanged groups are INCLUDED (callers filter), so the diff also
    // documents what did not move.
    TArray<FCk_SnapshotInspection_DiffEntityGroup> _EntityGroups;

    // Whole-save per-type aggregates. Sorted: |bytes delta| descending, then |count delta| descending, then type
    // path ascending.
    TArray<FCk_SnapshotInspection_DiffPayloadTypeRow> _PayloadTypes;

public:
    auto Get_GroupCountOfKind(ECk_SnapshotInspection_DiffGroupKind InKind) const -> int32;

public:
    CK_PROPERTY(_Valid);
    CK_PROPERTY(_InvalidReason);
    CK_PROPERTY(_EntityCountBaseline);
    CK_PROPERTY(_EntityCountCurrent);
    CK_PROPERTY(_PayloadCountBaseline);
    CK_PROPERTY(_PayloadCountCurrent);
    CK_PROPERTY(_SnapshotBytesBaseline);
    CK_PROPERTY(_SnapshotBytesCurrent);
    CK_PROPERTY(_Census);
    CK_PROPERTY(_EntityGroups);
    CK_PROPERTY(_PayloadTypes);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    /** Compares two already-inspected saves by durable identity, never by saved id (raw entt ids are not stable
     *  across captures).
     *
     *  Identity path contract — the matching key, per entity:
     *  - Local identity by provenance:
     *      EngineOwned      -> "key:<guid digits>" when the SaveKey is set, else "player:<PlayerId>" when
     *                          non-empty, else "engine:?" (unmatchable rows compare by count within their owner).
     *      ConstructSpawned -> "label:<Label>"
     *      RuntimeSpawned   -> "script:<ScriptClassPath>|actor:<ActorClassPath>|label:<Label>"
     *      DefinitionBuilt  -> "built:<step script paths joined with '+'>|label:<Label>"
     *  - Full path = owner's path + "/" + local identity. The owner hop is the lifetime owner when its row exists
     *    in the table, else the context owner when its row exists AND is not the entity itself (self-context is
     *    the normal "own context root" idiom, treated as a root), else the root marker "~" (owned by a
     *    non-persisted entity — the transient root). The walk is iterative and memoized; a row inside an owner
     *    cycle prefixes its path with "cycle~" at the point of re-entry, so a cyclic save still diffs without
     *    hanging.
     *
     *  Group kind: Added (baseline count 0), Removed (current count 0), CountChanged (both non-zero, different),
     *  PayloadsChanged (counts equal but a per-type payload count, byte total, or hash multiset differs),
     *  Unchanged. */
    CKSNAPSHOT_API auto
    Diff_Documents(
        const FCk_SnapshotInspection_Document& InBaseline,
        const FCk_SnapshotInspection_Document& InCurrent) -> FCk_SnapshotInspection_Diff;

    /** The identity path Diff_Documents assigns to one entity row — exposed so a UI can map a selected entity to
     *  its diff group and so tests pin the path contract directly. INDEX_NONE-safe: returns an empty string for
     *  an invalid index. */
    CKSNAPSHOT_API auto
    Get_IdentityPathForEntity(
        const FCk_SnapshotInspection_Document& InDocument,
        int32 InEntityIndex) -> FString;

    CKSNAPSHOT_API auto
    Get_DiffGroupKindText(
        ECk_SnapshotInspection_DiffGroupKind InKind) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
