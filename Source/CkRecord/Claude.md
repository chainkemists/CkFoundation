# CkRecord

**Purpose:** Entity parent/child record system. A Record entity owns a list of Record Entry entities. Each entry is a full entity with its own fragments. Used by every feature module that needs "one entity holding multiple child entities" (e.g., a character entity holding multiple audio track entities).

**Depends on:** `CkCore`, `CkEcs`, `CkLabel`, `CkLog`.
**Used by:** ~40 modules — every feature that has a Record type (Audio tracks, Attribute sets, VFX layers, objectives, etc.) uses `CkRecord`.

---

## Core types

```
FFragment_Record_*           – fragment on the parent entity that stores the entry list
FFragment_RecordEntry_*      – fragment on a child entity marking it as an entry
TUtils_RecordOfEntities<T>   – CRTP template base for Record utility classes
UCk_Utils_RecordEntry_UE     – base utility class for Record Entry operations
```

Each feature module defines its own concrete record type:

```cpp
// _Fragment_Data.h
USTRUCT() struct FFragment_AudioTrack_Record { /* ... TArray<FCk_Handle_AudioTrack> entries */ };

// _Utils.h
class UCk_Utils_AudioTrack_Record_UE : public ck::TUtils_RecordOfEntities<...> { };
```

---

## Key `TUtils_RecordOfEntities<T>` API

All resolved against a Record handle:

```cpp
static void AddIfMissing(FCk_Handle& InHandle, ECk_Record_EntryHandlingPolicy = Default);
static bool Has(const FCk_Handle& InHandle);
static bool Ensure(const FCk_Handle& InHandle); // asserts if missing

// Count
static int32 Get_ValidEntriesCount(const FCk_Handle& InRecordHandle);

// Presence
static bool Get_ContainsEntry(const FCk_Handle& InRecordHandle, const Handle& InEntry);

// Iteration
static TArray<Handle> Get_Entries(const FCk_Handle&);       // all (incl. invalid)
static TArray<Handle> Get_ValidEntries(const FCk_Handle&);  // valid only

// Predicate iteration (template)
static TArray<Handle> Get_ValidEntries_If(const FCk_Handle&, Predicate);
```

---

## Pattern: adding an entry to a record

Every feature module adds entries through its own typed utility (which delegates to `TUtils_RecordOfEntities`). The pattern is always:

1. Ensure the parent entity has the Record fragment (`AddIfMissing`).
2. Create a new entity with the entry fragment via `UCk_Utils_EntityLifetime_UE::Request_SpawnEntity`.
3. Label the entry with `UCk_Utils_GameplayLabel_UE::Add(EntryHandle, MyLabel)`.

```cpp
// Pseudo-code from a feature Utils
UCk_Utils_AudioTrack_Record::AddIfMissing(InOwnerHandle);
auto TrackHandle = /* spawn track entity */;
UCk_Utils_GameplayLabel_UE::Add(TrackHandle, Tag_AudioTrack_BGM);
```

---

## Entry handling policy

`ECk_Record_EntryHandlingPolicy` controls what happens when an entry is added to a record that already has a conflicting entry (same label, for instance):

- `Default` — framework decides based on the record type's definition.
- Override variants exist per record type (see each feature module's fragment data header).

---

## Meta Fragments

A "Meta Fragment" is a fragment that is itself a Record entry — the entry has its own EntityScript and fragments. Externally it looks like a plain fragment. Examples: `CkAttribute`'s `MeterAttribute` (which is internally a float-attribute entity stored in the attribute Record). `CkEcsExt` manages the parameter queuing needed for correct Meta Fragment construction on clients.

---

## Anti-patterns

1. Don't iterate `Get_Entries` when you only care about live entries — use `Get_ValidEntries` to skip destroyed entries that haven't been pruned yet.
2. Don't add entries directly to the internal fragment array. Always go through the feature module's `TUtils_RecordOfEntities`-derived utility — it maintains consistency and fires the right signals.
3. Don't skip labeling entries. Labels are the only way to distinguish "which BGM track" from "which SFX track" in the same record. Unlabeled entries are discoverable only by type, which breaks when a record has multiple entries of the same type.

---

## See also
- `CkLabel/Claude.md` — how entries are identified by role.
- `CkEcs/Claude.md` — entity creation / destruction primitives.
- `CkEcsExt/Claude.md` — Meta Fragment infrastructure and cross-record lookup.
