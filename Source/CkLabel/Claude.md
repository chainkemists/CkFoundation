# CkLabel

**Purpose:** Per-entity named label — a single `FGameplayTag` fragment that identifies an entity's role or type within its parent context. Think of it as "what kind of thing is this entity?" from the perspective of its owner.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`.
**Used by:** ~40 modules — any feature that needs to distinguish entity roles uses `CkLabel`.

---

## Core API (`UCk_Utils_GameplayLabel_UE`, `CkLabel_Utils.h`)

All methods are ScriptMixin on `FCk_Handle`, so they appear as member-style from BP/AS:

```cpp
// Add/overwrite a label on an entity
static void Add(FCk_Handle& InHandle, FGameplayTag InLabel);

// Check existence
static bool Has(const FCk_Handle& InHandle);   // has any label
static bool Ensure(const FCk_Handle& InHandle); // has label + fires ensure on missing

// Read
static FGameplayTag Get_Label(const FCk_Handle& InHandle);
static bool         Get_IsUnnamedLabel(const FCk_Handle& InHandle); // label == None

// Match
static bool Matches     (const FCk_Handle& InHandle, FGameplayTag InTagToMatch); // hierarchical
static bool MatchesExact(const FCk_Handle& InHandle, FGameplayTag InTagToMatch);
static bool MatchesAny  (const FCk_Handle& InHandle, const FGameplayTagContainer& InTagsToMatch);
```

---

## Concept: label vs. tag

- **Label** — one tag per entity, identifying its *role* within a parent structure (e.g., `Audio.Track.BGM` on a track entity owned by a music system entity). An entity has at most one label.
- **TagSet** (see `CkTagSet/`) — arbitrary set of tags on an entity for filtering/filtering behaviors (e.g., "this entity is `Flammable`, `Heavy`, `Interactable`"). An entity can have many tags.
- **GameplayTag fragments on features** — per-feature tags on specific fragments (e.g., `AudioTrack.State`). Those are owned by the feature module, not by `CkLabel`.

When a system iterates Record entries and wants to distinguish "which entry is the BGM track," the label is the answer.

---

## Pattern: label-based lookup in Records

Labels are the lookup key when iterating a Record's entries. The combination of `TUtils_RecordOfEntities` + `UCk_Utils_GameplayLabel_UE::MatchesExact` finds a specific entry:

```cpp
const auto Entries = TUtils_MyFeatureRecord::Get_ValidEntries(InRecordHandle);
for (const auto& EntryHandle : Entries)
{
    CK_ENSURE_IF_NOT(UCk_Utils_GameplayLabel_UE::Has(EntryHandle), TEXT("Unlabeled entry")) { continue; }

    if (UCk_Utils_GameplayLabel_UE::MatchesExact(EntryHandle, Tag_BGM))
    {
        // found the BGM entry
    }
}
```

---

## Depends on
`CkCore`, `CkEcs`, `CkLog`.

## Used by
Every module that uses `CkRecord` (parent/child entity trees) to tell entries apart. Also used by `CkEcsExt::Get_EntityOrRecordEntry_WithFragmentAndLabel`.

## See also
- `CkRecord/Claude.md` — the parent/child record system; labels are how entries are identified within a record.
- `CkTagSet/Claude.md` — for multiple tags per entity (behavior classification, not role identification).
- `CkCore/GameplayTag/README.md` — tag container utilities and requirements.
