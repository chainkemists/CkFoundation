#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Bake/CkGroundNav_Fingerprint.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>
#include <Misc/ScopeRWLock.h>
#include <UObject/WeakObjectPtr.h>

#include <atomic>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_world_fields
{
    using namespace ck::groundnav;
    using namespace ck::groundnav::world_fields;

    struct FEntry
    {
        FCk_Handle _VolumeEntity;
        FCk_GroundNav_FieldPtr _Field;
        TMap<FGameplayTag, FCk_GroundNav_FieldPtr> _VariantFields;
        FCk_GroundNav_PublishNote _PublishNote;
        TMap<FGameplayTag, FCk_GroundNav_PublishNote> _VariantPublishNotes;
        uint64 _Generation = 1;
    };

    struct FStreamTileState
    {
        FCk_GroundNav_TileCoord _Coord;
        TArray<uint8> _DefaultBlob;
        TMap<FGameplayTag, TArray<uint8>> _VariantBlobs;
        bool _Enabled = true;
    };

    struct FStreamSource
    {
        FCk_GroundNav_StreamSourceHandle _Handle;
        TMap<int32, FStreamTileState> _Tiles;
    };

    struct FStreamOwner
    {
        FCk_GroundNav_VolumeId _VolumeId;
        FCk_Handle _OwnerEntity;
        FCk_GroundNav_StreamFieldBundle _Bundle;
        TMap<uint64, FStreamSource> _Sources;
        uint64 _Generation = 1;
    };

    using FWorldStreamOwners = TMap<TWeakObjectPtr<UWorld>, TMap<FCk_GroundNav_VolumeId, FStreamOwner>>;

    auto Get_StreamOwners() -> FWorldStreamOwners&
    {
        static auto Owners = FWorldStreamOwners{};
        return Owners;
    }

    auto DoGet_OwnerEntry(TArray<FEntry>& InEntries, const FCk_Handle& InOwner) -> FEntry*
    {
        for (auto& Entry : InEntries)
        {
            if (Entry._VolumeEntity == InOwner)
            { return &Entry; }
        }

        return nullptr;
    }

    auto DoGet_TileBounds(const FCk_GroundNav_Field& InField, int32 InTileIndex) -> FBox
    {
        return Get_TileBounds(InField._Params.Get_TileBakeParams(
            Get_TileCoord(InField._Params._Divisions, InTileIndex), FCk_GroundNav_Epoch{}));
    }

    auto DoGet_ChangedBounds(const FCk_GroundNav_Field& InField, const TArray<int32>& InTileIndices) -> FBox
    {
        auto Bounds = FBox{ForceInit};
        for (const auto TileIndex : InTileIndices)
        {
            if (InField._Tiles.IsValidIndex(TileIndex))
            { Bounds += DoGet_TileBounds(InField, TileIndex); }
        }
        return Bounds;
    }

    auto DoValidate_Bundle(const FCk_GroundNav_StreamFieldBundle& InBundle) -> bool
    {
        const auto& Default = InBundle._DefaultField;
        if (NOT Default._Params.Get_IsValid() || Default._Tiles.Num() != Default._Params.Get_TileCount())
        { return false; }

        auto DefaultBuilt = TSet<int32>{};
        for (auto Index = 0; Index < Default._Tiles.Num(); ++Index)
        {
            if (Default._Tiles[Index]._Coord != Get_TileCoord(Default._Params._Divisions, Index)) { return false; }
            if (Default._Tiles[Index].Get_IsBuilt()) { DefaultBuilt.Add(Index); }
        }
        for (const auto& Variant : InBundle._VariantFields)
        {
            if (NOT Variant.Key.IsValid() || NOT Variant.Value._Params.Get_IsValid() ||
                Variant.Value._Tiles.Num() != Variant.Value._Params.Get_TileCount() ||
                Variant.Value._Params._Divisions != Default._Params._Divisions ||
                Variant.Value._Params._OriginXY != Default._Params._OriginXY ||
                Variant.Value._Params._MinZUu != Default._Params._MinZUu ||
                Variant.Value._Params._MaxZUu != Default._Params._MaxZUu)
            { return false; }

            // Frozen authored-input fingerprint is the canonical equality for otherwise private
            // config, merge, markup, link, and profile values.
            auto VariantBuilt = TSet<int32>{};
            for (auto Index = 0; Index < Variant.Value._Tiles.Num(); ++Index)
            {
                if (Variant.Value._Tiles[Index]._Coord != Get_TileCoord(Variant.Value._Params._Divisions, Index)) { return false; }
                if (Variant.Value._Tiles[Index].Get_IsBuilt()) { VariantBuilt.Add(Index); }
            }
            if (VariantBuilt.Num() != DefaultBuilt.Num()) { return false; }
            for (const auto Index : VariantBuilt)
            { if (!DefaultBuilt.Contains(Index)) { return false; } }
        }

        return true;
    }

    // A streamed owner keeps the lattice and each profile's bake identity, while complete derived
    // bundles are allowed to replace the authored markup/link records and the cost/link data they
    // produce. Those records deliberately do not participate in this identity check.
    auto DoGet_StreamOwnerIdentityFingerprint(const FCk_GroundNav_Field& InField) -> FCk_GroundNav_ContentFingerprint
    {
        return Get_InputFingerprint(InField._Params.Get_Bounds(), InField._Params._Config, InField._Params._Profile,
            {}, {}, InField._Params._MergeTunables, InField._Params._MaxClearanceUu);
    }

    auto DoBundlesMatchCanonical(const FCk_GroundNav_StreamFieldBundle& A, const FCk_GroundNav_StreamFieldBundle& B) -> bool
    {
        if (NOT DoValidate_Bundle(A) || NOT DoValidate_Bundle(B) || A._VariantFields.Num() != B._VariantFields.Num() ||
            A._DefaultField._Params._Divisions != B._DefaultField._Params._Divisions ||
            A._DefaultField._Params._OriginXY != B._DefaultField._Params._OriginXY ||
            A._DefaultField._Params._MinZUu != B._DefaultField._Params._MinZUu || A._DefaultField._Params._MaxZUu != B._DefaultField._Params._MaxZUu ||
            DoGet_StreamOwnerIdentityFingerprint(A._DefaultField) != DoGet_StreamOwnerIdentityFingerprint(B._DefaultField)) { return false; }
        for (const auto& Variant : A._VariantFields)
        {
            const auto* Other = B._VariantFields.Find(Variant.Key);
            if (Other == nullptr ||
                DoGet_StreamOwnerIdentityFingerprint(Variant.Value) != DoGet_StreamOwnerIdentityFingerprint(*Other))
            { return false; }
        }
        return true;
    }

    auto DoGet_MaxEpoch(const FCk_GroundNav_StreamFieldBundle& InBundle) -> FCk_GroundNav_Epoch
    {
        auto Epoch = InBundle._DefaultField._Epoch;
        for (const auto& Variant : InBundle._VariantFields)
        {
            if (Variant.Value._Epoch.Get_IsNewerThan(Epoch))
            { Epoch = Variant.Value._Epoch; }
        }
        return Epoch;
    }

    auto DoMake_ReplaceTransition(
        FCk_GroundNav_VolumeId InVolumeId,
        int32 InTileIndex,
        const FCk_GroundNav_Field& InField,
        const FStreamTileState& InTile) -> FCk_GroundNav_StreamTileTransition
    {
        auto Transition = FCk_GroundNav_StreamTileTransition{};
        Transition._TileId = FCk_GroundNav_StreamTileId{InVolumeId, Get_TileCoord(InField._Params._Divisions, InTileIndex)};
        Transition._Kind = ECk_GroundNav_StreamTileTransitionKind::Replace;
        Transition._DefaultBlob = InTile._DefaultBlob;
        Transition._VariantBlobs = InTile._VariantBlobs;
        return Transition;
    }

    auto DoMake_RemoveTransition(
        FCk_GroundNav_VolumeId InVolumeId,
        int32 InTileIndex,
        const FCk_GroundNav_Field& InField) -> FCk_GroundNav_StreamTileTransition
    {
        auto Transition = FCk_GroundNav_StreamTileTransition{};
        Transition._TileId = FCk_GroundNav_StreamTileId{InVolumeId, Get_TileCoord(InField._Params._Divisions, InTileIndex)};
        Transition._Kind = ECk_GroundNav_StreamTileTransitionKind::Remove;
        return Transition;
    }

    auto DoSet_CompositionFailure(
        FCk_GroundNav_StreamRegistryResult& OutResult,
        const FCk_GroundNav_StreamCompositionResult& InComposition) -> void
    {
        OutResult._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
        OutResult._CompositionStatus = InComposition._Status;
        OutResult._LoadStatus = InComposition._LoadStatus;
        OutResult._ProfileTag = InComposition._ProfileTag;
    }

    using FWorldEntries = TMap<TWeakObjectPtr<UWorld>, TArray<FEntry>>;

    auto Get_Entries() -> FWorldEntries&
    {
        static auto Entries = FWorldEntries{};
        return Entries;
    }

    // Per world, keyed and cleared exactly as the entries are: the epoch sums of fields whose volumes
    // are gone, so the surface revision keeps counting ground that no longer exists.
    auto Get_RetiredRevisions() -> TMap<TWeakObjectPtr<UWorld>, int64>&
    {
        static auto RetiredRevisions = TMap<TWeakObjectPtr<UWorld>, int64>{};
        return RetiredRevisions;
    }

    // The lock guards pointer handoff plus the streamed-owner sidecar identity. A reader copies a
    // shared pointer out and then queries the immutable field with no lock held; expensive field
    // validation, decode, serialization, and composition all operate on an optimistic snapshot after
    // releasing this lock.
    auto Get_Lock() -> FRWLock&
    {
        static auto Lock = FRWLock{};
        return Lock;
    }

    /**
     * The entry a location answers from: the one whose field contains it, or the first field there is.
     *
     * Said ONCE, so the field and the note beside it are never selected by two rules that could
     * disagree. Callers hold the lock; nothing here takes one.
     */
    auto TryGet_EntryFor(
        const TArray<FEntry>& InEntries,
        const FVector&        InLocation) -> const FEntry*
    {
        const FEntry* FirstWithField = nullptr;

        for (const auto& Entry : InEntries)
        {
            if (NOT Entry._Field.IsValid())
            { continue; }

            if (FirstWithField == nullptr)
            { FirstWithField = &Entry; }

            if (Entry._Field->_Params.Get_Bounds().IsInsideOrOn(InLocation))
            { return &Entry; }
        }

        return FirstWithField;
    }

    /**
     * Folds one publish into an entry's note.
     *
     * A GEOMETRY publish restarts the run: ground moved, and the ids accumulated so far describe none
     * of it. A link-only publish extends it, which is what lets a reader whose snapshot predates
     * several of them still answer by identity rather than falling back to bounds. A cost-only publish
     * moves the epoch and touches neither: the ground it re-priced is the ground that was already
     * published, so the run the note is keeping is still the whole of what a reader has missed.
     */
    auto DoApply_Publish(
        ck::groundnav::world_fields::FCk_GroundNav_PublishNote&        InOutNote,
        const ck::groundnav::FCk_GroundNav_Epoch&                      InPublishedEpoch,
        const ck::groundnav::world_fields::FCk_GroundNav_PublishClaim& InClaim) -> void
    {
        InOutNote._Epoch = InPublishedEpoch;

        if (InClaim.Get_Kind() == ck::groundnav::world_fields::ECk_GroundNav_PublishKind::Geometry)
        {
            InOutNote._LastGeometryEpoch = InPublishedEpoch;
            InOutNote._ChangedLinkEpochsSinceGeometry.Reset();

            return;
        }

        if (InClaim.Get_Kind() == ck::groundnav::world_fields::ECk_GroundNav_PublishKind::CostOnly)
        { return; }

        for (const auto ChangedLinkId : InClaim.Get_ChangedLinkIds())
        { InOutNote._ChangedLinkEpochsSinceGeometry.Add(ChangedLinkId, InPublishedEpoch); }
    }

    auto DoApply_PublishNotes(
        FEntry& InOutEntry,
        const ck::groundnav::world_fields::FCk_GroundNav_PublishClaim& InClaim) -> void
    {
        const auto DefaultEpoch = InOutEntry._Field.IsValid()
            ? InOutEntry._Field->_Epoch
            : ck::groundnav::FCk_GroundNav_Epoch{};
        DoApply_Publish(InOutEntry._PublishNote, DefaultEpoch, InClaim);

        for (auto It = InOutEntry._VariantPublishNotes.CreateIterator(); It; ++It)
        {
            if (NOT InOutEntry._VariantFields.Contains(It.Key()))
            { It.RemoveCurrent(); }
        }

        for (const auto& VariantField : InOutEntry._VariantFields)
        {
            if (NOT VariantField.Value.IsValid())
            { continue; }

            DoApply_Publish(
                InOutEntry._VariantPublishNotes.FindOrAdd(VariantField.Key), VariantField.Value->_Epoch, InClaim);
        }
    }

    auto DoRetire_ReplacedVariants(
        UWorld* InWorld,
        const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InOldVariantFields,
        const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InNewVariantFields) -> void;

    /**
     * Retires the tile-epoch sums a new variant map is about to take out of the live count.
     *
     * A tag the new map does not hold, and a tag it holds a DIFFERENT field for, are both sums that stop
     * being counted the moment the map is replaced. Retiring them is what keeps a surface revision
     * monotone across a variant being dropped or rebaked, exactly as Unpublish keeps it monotone across
     * a volume going away. Callers hold the write lock; nothing here takes one.
     */
    auto DoRetire_ReplacedVariants(
        UWorld*                                                          InWorld,
        const TMap<FGameplayTag, ck::groundnav::FCk_GroundNav_FieldPtr>& InOldVariantFields,
        const TMap<FGameplayTag, ck::groundnav::FCk_GroundNav_FieldPtr>& InNewVariantFields) -> void
    {
        if (InOldVariantFields.IsEmpty())
        { return; }

        auto& RetiredRevision = Get_RetiredRevisions().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});

        for (const auto& OldVariantField : InOldVariantFields)
        {
            if (NOT OldVariantField.Value.IsValid())
            { continue; }

            const auto* NewVariantField = InNewVariantFields.Find(OldVariantField.Key);

            if (NewVariantField != nullptr && *NewVariantField == OldVariantField.Value)
            { continue; }

            RetiredRevision += OldVariantField.Value->Get_AggregatedTileEpochSum();
        }
    }

    auto DoBind_CleanupHookOnce() -> void
    {
        static auto CleanupHookIsBound = false;

        if (CleanupHookIsBound)
        { return; }

        CleanupHookIsBound = true;

        FWorldDelegates::OnWorldCleanup.AddLambda(
            [](UWorld* InWorld, bool /*InSessionEnded*/, bool /*InCleanupResources*/) -> void
            {
                auto Lock = FRWScopeLock{Get_Lock(), SLT_Write};
                Get_Entries().Remove(TWeakObjectPtr<UWorld>{InWorld});
                Get_RetiredRevisions().Remove(TWeakObjectPtr<UWorld>{InWorld});
                Get_StreamOwners().Remove(TWeakObjectPtr<UWorld>{InWorld});
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Publish(
        UWorld*                                           InWorld,
        const FCk_Handle&                                 InVolumeEntity,
        FCk_GroundNav_FieldPtr                            InField,
        const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InVariantFields,
        const FCk_GroundNav_PublishClaim&                 InClaim)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    ck_groundnav_world_fields::DoBind_CleanupHookOnce();

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_Write};

    auto& Entries = ck_groundnav_world_fields::Get_Entries().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});

    for (auto& Entry : Entries)
    {
        if (Entry._VolumeEntity != InVolumeEntity)
        { continue; }

        // Retired BEFORE the map goes, because the sums that are about to stop being counted can only
        // be read off the map that still holds them.
        ck_groundnav_world_fields::DoRetire_ReplacedVariants(
            InWorld, Entry._VariantFields, InVariantFields);

        Entry._Field = MoveTemp(InField);
        Entry._VariantFields = InVariantFields;

        ck_groundnav_world_fields::DoApply_PublishNotes(Entry, InClaim);
        ++Entry._Generation;

        return;
    }

    auto NewEntry = ck_groundnav_world_fields::FEntry{
        InVolumeEntity, MoveTemp(InField), InVariantFields, {}, {}};

    ck_groundnav_world_fields::DoApply_PublishNotes(NewEntry, InClaim);

    Entries.Emplace(MoveTemp(NewEntry));
}

auto ck::groundnav::world_fields::Publish(
    UWorld* InWorld, const FCk_Handle& InVolumeEntity, FCk_GroundNav_FieldPtr InField,
    const TMap<FGameplayTag, FCk_GroundNav_FieldPtr>& InVariantFields) -> void
{
    Publish(InWorld, InVolumeEntity, MoveTemp(InField), InVariantFields, FCk_GroundNav_PublishClaim::Geometry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Unpublish(
        UWorld*           InWorld,
        const FCk_Handle& InVolumeEntity)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_Write};

    auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries != nullptr)
    {
        auto& RetiredRevision = ck_groundnav_world_fields::Get_RetiredRevisions().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});

        Entries->RemoveAll([&](const ck_groundnav_world_fields::FEntry& InEntry) -> bool
        {
            if (InEntry._VolumeEntity != InVolumeEntity)
            { return false; }

            if (InEntry._Field.IsValid())
            { RetiredRevision += InEntry._Field->Get_AggregatedTileEpochSum(); }

            // The variants retire with the default and count exactly as it does: a world that keeps
            // counting ground that went away must keep counting every profile's copy of it, or the
            // revision falls the moment a volume holding variants tears down.
            for (const auto& VariantField : InEntry._VariantFields)
            {
                if (VariantField.Value.IsValid())
                { RetiredRevision += VariantField.Value->Get_AggregatedTileEpochSum(); }
            }

            return true;
        });
    }

    // A legacy lifecycle caller can still reach Unpublish directly. Drop every streamed sidecar
    // bound to this ECS owner in the same critical section so a later registration cannot inherit
    // retained blobs or a stale source handle from the retired entry.
    if (auto* WorldOwners = ck_groundnav_world_fields::Get_StreamOwners().Find(TWeakObjectPtr<UWorld>{InWorld}))
    {
        for (auto It = WorldOwners->CreateIterator(); It; ++It)
        {
            if (It.Value()._OwnerEntity == InVolumeEntity)
            { It.RemoveCurrent(); }
        }
        if (WorldOwners->IsEmpty())
        { ck_groundnav_world_fields::Get_StreamOwners().Remove(TWeakObjectPtr<UWorld>{InWorld}); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Get_RetiredRevision(
        UWorld* InWorld)
    -> int64
{
    if (ck::Is_NOT_Valid(InWorld))
    { return 0; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* RetiredRevision = ck_groundnav_world_fields::Get_RetiredRevisions().Find(TWeakObjectPtr<UWorld>{InWorld});

    return RetiredRevision == nullptr ? int64{0} : *RetiredRevision;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Get_VariantRevision(
        UWorld* InWorld)
    -> int64
{
    if (ck::Is_NOT_Valid(InWorld))
    { return 0; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries == nullptr)
    { return 0; }

    auto Revision = int64{0};

    for (const auto& Entry : *Entries)
    {
        for (const auto& VariantField : Entry._VariantFields)
        {
            if (NOT VariantField.Value.IsValid())
            { continue; }

            Revision += VariantField.Value->Get_AggregatedTileEpochSum();
        }
    }

    return Revision;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    TryGet_Field(
        UWorld*        InWorld,
        const FVector& InLocation)
    -> FCk_GroundNav_FieldPtr
{
    if (ck::Is_NOT_Valid(InWorld))
    { return {}; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries == nullptr)
    { return {}; }

    const auto* Entry = ck_groundnav_world_fields::TryGet_EntryFor(*Entries, InLocation);

    return Entry == nullptr ? FCk_GroundNav_FieldPtr{} : Entry->_Field;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    TryGet_Field(
        UWorld*             InWorld,
        const FVector&      InLocation,
        const FGameplayTag& InProfileTag)
    -> FCk_GroundNav_FieldPtr
{
    if (ck::Is_NOT_Valid(InWorld))
    { return {}; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries == nullptr)
    { return {}; }

    const auto* Entry = ck_groundnav_world_fields::TryGet_EntryFor(*Entries, InLocation);

    if (Entry == nullptr)
    { return {}; }

    if (NOT InProfileTag.IsValid())
    { return Entry->_Field; }

    const auto* VariantField = Entry->_VariantFields.Find(InProfileTag);

    // No fallback: a volume that authored no variant under this tag has no ground for that profile,
    // and the default's is a different world to walk in.
    return VariantField == nullptr ? FCk_GroundNav_FieldPtr{} : *VariantField;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    TryGet_PublishNote(
        UWorld*             InWorld,
        const FVector&      InLocation,
        const FGameplayTag& InProfileTag)
    -> TOptional<FCk_GroundNav_PublishNote>
{
    if (ck::Is_NOT_Valid(InWorld))
    { return {}; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries == nullptr)
    { return {}; }

    const auto* Entry = ck_groundnav_world_fields::TryGet_EntryFor(*Entries, InLocation);

    if (Entry == nullptr)
    { return {}; }

    if (NOT InProfileTag.IsValid())
    { return Entry->_PublishNote; }

    const auto* VariantNote = Entry->_VariantPublishNotes.Find(InProfileTag);
    return VariantNote == nullptr ? TOptional<FCk_GroundNav_PublishNote>{} : *VariantNote;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Get_Fields(
        UWorld* InWorld)
    -> TArray<FCk_GroundNav_FieldPtr>
{
    auto Fields = TArray<FCk_GroundNav_FieldPtr>{};

    if (ck::Is_NOT_Valid(InWorld))
    { return Fields; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries == nullptr)
    { return Fields; }

    Fields.Reserve(Entries->Num());

    for (const auto& Entry : *Entries)
    {
        if (NOT Entry._Field.IsValid())
        { continue; }

        Fields.Emplace(Entry._Field);
    }

    return Fields;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Get_FieldCount(
        UWorld* InWorld)
    -> int32
{
    return Get_Fields(InWorld).Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Get_VolumeEntities(
        UWorld* InWorld)
    -> TArray<FCk_Handle>
{
    auto VolumeEntities = TArray<FCk_Handle>{};

    if (ck::Is_NOT_Valid(InWorld))
    { return VolumeEntities; }

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};

    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});

    if (Entries == nullptr)
    { return VolumeEntities; }

    VolumeEntities.Reserve(Entries->Num());

    for (const auto& Entry : *Entries)
    { VolumeEntities.Emplace(Entry._VolumeEntity); }

    return VolumeEntities;
}

auto ck::groundnav::world_fields::TryGet_FieldSnapshot(
    UWorld* InWorld, const FVector& InLocation, const FGameplayTag& InProfileTag)
    -> TOptional<FCk_GroundNav_FieldSnapshot>
{
    if (ck::Is_NOT_Valid(InWorld)) { return {}; }
    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};
    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});
    const auto* Entry = Entries == nullptr ? nullptr : ck_groundnav_world_fields::TryGet_EntryFor(*Entries, InLocation);
    if (Entry == nullptr) { return {}; }
    auto Snapshot = FCk_GroundNav_FieldSnapshot{};
    if (NOT InProfileTag.IsValid()) { Snapshot._Field = Entry->_Field; Snapshot._PublishNote = Entry->_PublishNote; }
    else
    {
        const auto* Field = Entry->_VariantFields.Find(InProfileTag);
        const auto* Note = Entry->_VariantPublishNotes.Find(InProfileTag);
        if (Field == nullptr || Note == nullptr) { return {}; }
        Snapshot._Field = *Field; Snapshot._PublishNote = *Note;
    }
    return Snapshot._Field.IsValid() ? TOptional<FCk_GroundNav_FieldSnapshot>{MoveTemp(Snapshot)} : TOptional<FCk_GroundNav_FieldSnapshot>{};
}

namespace ck::groundnav::world_fields
{
    struct FCk_GroundNav_StreamRegistryAccess
    {
        static auto Make(uint64 InValue) -> FCk_GroundNav_StreamSourceHandle { return FCk_GroundNav_StreamSourceHandle{InValue}; }
        static auto Get(const FCk_GroundNav_StreamSourceHandle& InHandle) -> uint64 { return InHandle._Value; }
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    using namespace ck::groundnav;
    using namespace ck::groundnav::world_fields;

    auto DoMake_Result(ECk_GroundNav_StreamRegistryStatus InStatus) -> FCk_GroundNav_StreamRegistryResult
    {
        auto Result = FCk_GroundNav_StreamRegistryResult{};
        Result._Status = InStatus;
        return Result;
    }

    auto DoFind_StreamOwner(UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId) -> ck_groundnav_world_fields::FStreamOwner*
    {
        auto* WorldOwners = ck_groundnav_world_fields::Get_StreamOwners().Find(TWeakObjectPtr<UWorld>{InWorld});
        return WorldOwners == nullptr ? nullptr : WorldOwners->Find(InVolumeId);
    }

    auto DoAllocate_StreamSourceHandle() -> FCk_GroundNav_StreamSourceHandle
    {
        static std::atomic<uint64> NextValue{1};

        auto Value = NextValue.fetch_add(1, std::memory_order_relaxed);
        while (Value == 0)
        { Value = NextValue.fetch_add(1, std::memory_order_relaxed); }

        return FCk_GroundNav_StreamRegistryAccess::Make(Value);
    }

    struct FStreamOwnerTransactionSnapshot
    {
        FCk_GroundNav_VolumeId _VolumeId;
        FCk_Handle _OwnerEntity;
        TMap<uint64, ck_groundnav_world_fields::FStreamSource> _Sources;
        ck_groundnav_world_fields::FEntry _Entry;
        uint64 _OwnerGeneration = 0;
        uint64 _EntryGeneration = 0;
    };

    struct FPreparedStreamOwnerTransaction
    {
        ck_groundnav_world_fields::FStreamOwner _Owner;
        ck_groundnav_world_fields::FEntry _Entry;
        uint64 _ExpectedOwnerGeneration = 0;
        uint64 _ExpectedEntryGeneration = 0;
        bool _HasPublication = false;
        FCk_GroundNav_StreamRegistryResult _Result;
    };

    auto DoCopy_StreamOwnerTransactionSnapshot(
        UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId,
        FStreamOwnerTransactionSnapshot& OutSnapshot) -> ECk_GroundNav_StreamRegistryStatus
    {
        auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};
        const auto* Owner = DoFind_StreamOwner(InWorld, InVolumeId);
        if (Owner == nullptr) { return ECk_GroundNav_StreamRegistryStatus::OwnerNotRegistered; }

        const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});
        const auto* Entry = Entries == nullptr ? nullptr : [&]() -> const ck_groundnav_world_fields::FEntry*
        {
            for (const auto& Candidate : *Entries)
            {
                if (Candidate._VolumeEntity == Owner->_OwnerEntity)
                { return &Candidate; }
            }
            return nullptr;
        }();
        if (Entry == nullptr || NOT Entry->_Field.IsValid())
        { return ECk_GroundNav_StreamRegistryStatus::OwnerEntryMissing; }
        for (const auto& Variant : Entry->_VariantFields)
        {
            if (NOT Variant.Value.IsValid())
            { return ECk_GroundNav_StreamRegistryStatus::OwnerEntryMissing; }
        }

        OutSnapshot._VolumeId = Owner->_VolumeId;
        OutSnapshot._OwnerEntity = Owner->_OwnerEntity;
        OutSnapshot._Sources = Owner->_Sources;
        OutSnapshot._Entry = *Entry;
        OutSnapshot._OwnerGeneration = Owner->_Generation;
        OutSnapshot._EntryGeneration = Entry->_Generation;
        return ECk_GroundNav_StreamRegistryStatus::Published;
    }

    auto DoMake_CandidateOwner(
        const FStreamOwnerTransactionSnapshot& InSnapshot) -> ck_groundnav_world_fields::FStreamOwner
    {
        // The read-side snapshot carries immutable publication pointers. Their field values are copied
        // only after releasing the registry lock, alongside the subsequent decode and composition.
        auto Candidate = ck_groundnav_world_fields::FStreamOwner{};
        Candidate._VolumeId = InSnapshot._VolumeId;
        Candidate._OwnerEntity = InSnapshot._OwnerEntity;
        Candidate._Sources = InSnapshot._Sources;
        Candidate._Generation = InSnapshot._OwnerGeneration;
        Candidate._Bundle._DefaultField = *InSnapshot._Entry._Field;
        for (const auto& Variant : InSnapshot._Entry._VariantFields)
        { Candidate._Bundle._VariantFields.Emplace(Variant.Key, *Variant.Value); }
        return Candidate;
    }

    auto DoMake_AllTileIndices(const FCk_GroundNav_StreamFieldBundle& InBundle) -> TArray<int32>
    {
        auto Indices = TArray<int32>{};
        Indices.Reserve(InBundle._DefaultField._Tiles.Num());
        for (auto TileIndex = 0; TileIndex < InBundle._DefaultField._Tiles.Num(); ++TileIndex)
        { Indices.Emplace(TileIndex); }
        return Indices;
    }

    auto DoStamp_TileEpochs(
        FCk_GroundNav_StreamFieldBundle& InOutBundle,
        TConstArrayView<int32> InTileIndices,
        FCk_GroundNav_Epoch InEpoch) -> void
    {
        for (const auto TileIndex : InTileIndices)
        {
            if (InOutBundle._DefaultField._Tiles.IsValidIndex(TileIndex))
            { InOutBundle._DefaultField._Tiles[TileIndex]._Epoch = InEpoch; }
            for (auto& Variant : InOutBundle._VariantFields)
            {
                if (Variant.Value._Tiles.IsValidIndex(TileIndex))
                { Variant.Value._Tiles[TileIndex]._Epoch = InEpoch; }
            }
        }
    }

    auto DoValidate_RetainedReplacement(
        const FCk_GroundNav_Field& InBaseField,
        const TArray<uint8>& InBlob,
        const FCk_GroundNav_TileCoord& InCoord) -> ECk_GroundNav_LoadStatus
    {
        auto Decode = InBaseField;
        for (auto TileIndex = 0; TileIndex < Decode._Tiles.Num(); ++TileIndex)
        {
            auto Unbuilt = FCk_GroundNav_Tile{};
            Unbuilt._Coord = Get_TileCoord(Decode._Params._Divisions, TileIndex);
            Decode._Tiles[TileIndex] = MoveTemp(Unbuilt);
        }
        const auto Status = Read_TileInto(InBlob, Decode, ECk_GroundNav_ComposeOnLoad::Deferred);
        const auto* Tile = Decode.Get_Tile(InCoord);
        return Status == ECk_GroundNav_LoadStatus::Loaded && Tile != nullptr &&
            Tile->_Coord == InCoord && Tile->Get_IsBuilt()
            ? ECk_GroundNav_LoadStatus::Loaded
            : Status == ECk_GroundNav_LoadStatus::Loaded
                ? ECk_GroundNav_LoadStatus::Corrupt
                : Status;
    }

    auto DoValidate_RetainedReplacement(
        const FCk_GroundNav_StreamFieldBundle& InBundle,
        FCk_GroundNav_VolumeId InVolumeId,
        const FCk_GroundNav_StreamTileTransition& InTransition,
        FCk_GroundNav_StreamRegistryResult& OutFailure) -> bool
    {
        const auto IsValidReplace = InTransition._TileId.Get_IsValid() &&
            InTransition._TileId._VolumeId == InVolumeId &&
            InTransition._Kind == ECk_GroundNav_StreamTileTransitionKind::Replace &&
            NOT InTransition._DefaultBlob.IsEmpty() &&
            InTransition._VariantBlobs.Num() == InBundle._VariantFields.Num();
        if (NOT IsValidReplace)
        {
            OutFailure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            OutFailure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
            return false;
        }

        const auto DefaultStatus = DoValidate_RetainedReplacement(
            InBundle._DefaultField, InTransition._DefaultBlob, InTransition._TileId._Coord);
        if (DefaultStatus != ECk_GroundNav_LoadStatus::Loaded)
        {
            OutFailure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            OutFailure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::BlobRefused;
            OutFailure._LoadStatus = DefaultStatus;
            return false;
        }

        for (const auto& Variant : InBundle._VariantFields)
        {
            const auto* Blob = InTransition._VariantBlobs.Find(Variant.Key);
            if (Blob == nullptr || Blob->IsEmpty())
            {
                OutFailure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
                OutFailure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
                OutFailure._ProfileTag = Variant.Key;
                return false;
            }
            const auto VariantStatus = DoValidate_RetainedReplacement(
                Variant.Value, *Blob, InTransition._TileId._Coord);
            if (VariantStatus != ECk_GroundNav_LoadStatus::Loaded)
            {
                OutFailure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
                OutFailure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::BlobRefused;
                OutFailure._LoadStatus = VariantStatus;
                OutFailure._ProfileTag = Variant.Key;
                return false;
            }
        }
        return true;
    }

    auto DoMake_PreparedStateOnlyTransaction(
        const FStreamOwnerTransactionSnapshot& InSnapshot,
        ck_groundnav_world_fields::FStreamOwner&& InCandidateOwner,
        FCk_GroundNav_StreamRegistryResult InResult) -> FPreparedStreamOwnerTransaction
    {
        auto Prepared = FPreparedStreamOwnerTransaction{};
        InCandidateOwner._Generation = InSnapshot._OwnerGeneration + 1;
        Prepared._Owner = MoveTemp(InCandidateOwner);
        Prepared._ExpectedOwnerGeneration = InSnapshot._OwnerGeneration;
        Prepared._ExpectedEntryGeneration = InSnapshot._EntryGeneration;
        Prepared._Result = MoveTemp(InResult);
        Prepared._Result._Epoch = InSnapshot._Entry._Field.IsValid()
            ? InSnapshot._Entry._Field->_Epoch
            : FCk_GroundNav_Epoch{};
        return Prepared;
    }

    auto DoTilesMatch(
        const ck_groundnav_world_fields::FStreamTileState& A,
        const ck_groundnav_world_fields::FStreamTileState& B) -> bool
    {
        if (A._Coord != B._Coord || A._Enabled != B._Enabled || A._DefaultBlob != B._DefaultBlob ||
            A._VariantBlobs.Num() != B._VariantBlobs.Num())
        { return false; }

        for (const auto& Variant : A._VariantBlobs)
        {
            const auto* OtherBlob = B._VariantBlobs.Find(Variant.Key);
            if (OtherBlob == nullptr || *OtherBlob != Variant.Value)
            { return false; }
        }

        return true;
    }

    auto DoSourcesMatch(
        const ck_groundnav_world_fields::FStreamSource& A,
        const ck_groundnav_world_fields::FStreamSource& B) -> bool
    {
        if (A._Handle != B._Handle || A._Tiles.Num() != B._Tiles.Num())
        { return false; }

        for (const auto& Tile : A._Tiles)
        {
            const auto* OtherTile = B._Tiles.Find(Tile.Key);
            if (OtherTile == nullptr || NOT DoTilesMatch(Tile.Value, *OtherTile))
            { return false; }
        }

        return true;
    }

    auto DoMake_PreparedPublishTransaction(
        const FStreamOwnerTransactionSnapshot& InSnapshot,
        ck_groundnav_world_fields::FStreamOwner&& InCandidateOwner,
        const FCk_GroundNav_PublishClaim& InClaim,
        const FBox& InChangedBounds,
        TConstArrayView<int32> InTileEpochIndices) -> FPreparedStreamOwnerTransaction
    {
        auto LatestEpoch = ck_groundnav_world_fields::DoGet_MaxEpoch(InCandidateOwner._Bundle);
        const auto ConsiderPublishedEpoch = [&LatestEpoch](const FCk_GroundNav_FieldPtr& InField) -> void
        {
            if (InField.IsValid() && InField->_Epoch.Get_IsNewerThan(LatestEpoch))
            { LatestEpoch = InField->_Epoch; }
        };
        ConsiderPublishedEpoch(InSnapshot._Entry._Field);
        for (const auto& Variant : InSnapshot._Entry._VariantFields)
        { ConsiderPublishedEpoch(Variant.Value); }
        const auto Epoch = LatestEpoch.Get_Next();
        InCandidateOwner._Bundle._DefaultField._Epoch = Epoch;
        for (auto& Variant : InCandidateOwner._Bundle._VariantFields)
        { Variant.Value._Epoch = Epoch; }
        DoStamp_TileEpochs(InCandidateOwner._Bundle, InTileEpochIndices, Epoch);

        auto Prepared = FPreparedStreamOwnerTransaction{};
        Prepared._Owner = MoveTemp(InCandidateOwner);
        Prepared._Owner._Generation = InSnapshot._OwnerGeneration + 1;
        Prepared._Entry = InSnapshot._Entry;
        Prepared._Entry._Field = MakeShared<const FCk_GroundNav_Field>(Prepared._Owner._Bundle._DefaultField);
        Prepared._Entry._VariantFields.Reset();
        for (const auto& Variant : Prepared._Owner._Bundle._VariantFields)
        {
            Prepared._Entry._VariantFields.Emplace(
                Variant.Key, MakeShared<const FCk_GroundNav_Field>(Variant.Value));
        }

        ck_groundnav_world_fields::DoApply_PublishNotes(Prepared._Entry, InClaim);
        Prepared._Entry._Generation = InSnapshot._EntryGeneration + 1;
        Prepared._ExpectedOwnerGeneration = InSnapshot._OwnerGeneration;
        Prepared._ExpectedEntryGeneration = InSnapshot._EntryGeneration;
        Prepared._HasPublication = true;
        Prepared._Result = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::Published);
        Prepared._Result._ChangedBounds = InChangedBounds;
        Prepared._Result._Epoch = Epoch;
        return Prepared;
    }

    auto DoCommit_PreparedStreamOwnerTransaction(
        UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId,
        FPreparedStreamOwnerTransaction&& InPrepared) -> FCk_GroundNav_StreamRegistryResult
    {
        auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_Write};
        auto* Owner = DoFind_StreamOwner(InWorld, InVolumeId);
        if (Owner == nullptr || Owner->_Generation != InPrepared._ExpectedOwnerGeneration)
        {
            return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::ConcurrentChange);
        }

        auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});
        auto* Entry = Entries == nullptr ? nullptr : ck_groundnav_world_fields::DoGet_OwnerEntry(*Entries, Owner->_OwnerEntity);
        if (Entry == nullptr || Entry->_Generation != InPrepared._ExpectedEntryGeneration)
        {
            return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::ConcurrentChange);
        }

        if (InPrepared._HasPublication)
        {
            // Retire only after both generations match. A refused or concurrent transaction therefore
            // leaves the published pointers, notes, and revision accounting untouched.
            ck_groundnav_world_fields::DoRetire_ReplacedVariants(
                InWorld, Entry->_VariantFields, InPrepared._Entry._VariantFields);
            *Entry = MoveTemp(InPrepared._Entry);
        }

        *Owner = MoveTemp(InPrepared._Owner);
        return MoveTemp(InPrepared._Result);
    }

    auto DoCompose_PreparePublish(
        const FStreamOwnerTransactionSnapshot& InSnapshot,
        ck_groundnav_world_fields::FStreamOwner&& InCandidateOwner,
        TArray<FCk_GroundNav_StreamTileTransition>&& InTransitions,
        const FCk_GroundNav_PublishClaim& InClaim,
        FCk_GroundNav_StreamRegistryResult& OutFailure,
        const TArray<int32>* InTileEpochIndices = nullptr) -> TOptional<FPreparedStreamOwnerTransaction>
    {
        InTransitions.Sort([](const FCk_GroundNav_StreamTileTransition& A, const FCk_GroundNav_StreamTileTransition& B)
        {
            return A._TileId._Coord._X != B._TileId._Coord._X
                ? A._TileId._Coord._X < B._TileId._Coord._X
                : A._TileId._Coord._Y < B._TileId._Coord._Y;
        });

        auto CandidateBundle = FCk_GroundNav_StreamFieldBundle{};
        const auto Composition = Compose_StreamTileTransitions(
            InCandidateOwner._Bundle, InCandidateOwner._VolumeId, InTransitions, CandidateBundle);
        const auto CompositionSucceeded = Composition.Get_Succeeded();
        CK_ENSURE_IF_NOT(CompositionSucceeded,
            TEXT("GroundNav stream transaction composition was refused"))
        {}
        if (NOT CompositionSucceeded)
        {
            ck_groundnav_world_fields::DoSet_CompositionFailure(OutFailure, Composition);
            return {};
        }

        auto ChangedIndices = TArray<int32>{};
        ChangedIndices.Reserve(InTransitions.Num());
        for (const auto& Transition : InTransitions)
        { ChangedIndices.Emplace(Get_TileIndex(InCandidateOwner._Bundle._DefaultField._Params._Divisions, Transition._TileId._Coord)); }

        auto ChangedBounds = ck_groundnav_world_fields::DoGet_ChangedBounds(
            InCandidateOwner._Bundle._DefaultField, ChangedIndices);
        for (const auto& Transition : InTransitions)
        {
            const auto TileIndex = Get_TileIndex(CandidateBundle._DefaultField._Params._Divisions, Transition._TileId._Coord);
            ChangedBounds += ck_groundnav_world_fields::DoGet_TileBounds(CandidateBundle._DefaultField, TileIndex);
        }

        InCandidateOwner._Bundle = MoveTemp(CandidateBundle);
        return DoMake_PreparedPublishTransaction(
            InSnapshot, MoveTemp(InCandidateOwner), InClaim, ChangedBounds,
            InTileEpochIndices == nullptr ? TConstArrayView<int32>{ChangedIndices} : TConstArrayView<int32>{*InTileEpochIndices});
    }
}

auto ck::groundnav::world_fields::Register_StreamOwner(
    UWorld* InWorld, const FCk_Handle& InOwnerEntity, FCk_GroundNav_VolumeId InVolumeId,
    const FCk_GroundNav_StreamFieldBundle& InInitialBundle,
    const bool InCreateBootstrapSource) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream registration requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }

    const auto OwnerIsValid = ck::IsValid(InOwnerEntity);
    CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("GroundNav stream registration requires a valid owner"))
    {}
    if (NOT OwnerIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidOwner); }

    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream registration requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }

    const auto BundleIsValid = ck_groundnav_world_fields::DoValidate_Bundle(InInitialBundle);
    CK_ENSURE_IF_NOT(BundleIsValid, TEXT("GroundNav stream registration requires a valid all-profile bundle"))
    {}
    if (NOT BundleIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidBundle); }

    // Registration is the first all-profile Geometry publication. Build both immutable publication
    // pointers and the bootstrap source before taking the write lock, so a refusal cannot leave an
    // owner sidecar or a half-published entry behind.
    auto InitialBundle = InInitialBundle;
    const auto InitialEpoch = ck_groundnav_world_fields::DoGet_MaxEpoch(InitialBundle).Get_Next();
    InitialBundle._DefaultField._Epoch = InitialEpoch;
    for (auto& Variant : InitialBundle._VariantFields)
    { Variant.Value._Epoch = InitialEpoch; }
    const auto InitialTileIndices = DoMake_AllTileIndices(InitialBundle);
    DoStamp_TileEpochs(InitialBundle, InitialTileIndices, InitialEpoch);

    auto Owner = ck_groundnav_world_fields::FStreamOwner{InVolumeId, InOwnerEntity, MoveTemp(InitialBundle)};
    auto Bootstrap = ck_groundnav_world_fields::FStreamSource{};
    auto BootstrapHandle = FCk_GroundNav_StreamSourceHandle{};
    if (InCreateBootstrapSource)
    {
        Bootstrap._Handle = DoAllocate_StreamSourceHandle();
        BootstrapHandle = Bootstrap._Handle;
        for (auto TileIndex = 0; TileIndex < Owner._Bundle._DefaultField._Tiles.Num(); ++TileIndex)
        {
            if (NOT Owner._Bundle._DefaultField._Tiles[TileIndex].Get_IsBuilt()) { continue; }
            auto Tile = ck_groundnav_world_fields::FStreamTileState{};
            const auto Coord = Get_TileCoord(Owner._Bundle._DefaultField._Params._Divisions, TileIndex);
            Tile._Coord = Coord;
            Write_Tile(Owner._Bundle._DefaultField, Coord, Tile._DefaultBlob);
            for (const auto& Variant : Owner._Bundle._VariantFields)
            {
                const auto VariantHasTile = Variant.Value._Tiles.IsValidIndex(TileIndex) &&
                    Variant.Value._Tiles[TileIndex].Get_IsBuilt();
                CK_ENSURE_IF_NOT(VariantHasTile,
                    TEXT("GroundNav stream registration bundle lost a variant tile during preparation"))
                {}
                if (NOT VariantHasTile) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidBundle); }
                Write_Tile(Variant.Value, Coord, Tile._VariantBlobs.FindOrAdd(Variant.Key));
            }
            Bootstrap._Tiles.Emplace(TileIndex, MoveTemp(Tile));
        }
        Owner._Sources.Emplace(FCk_GroundNav_StreamRegistryAccess::Get(Bootstrap._Handle), MoveTemp(Bootstrap));
    }
    const auto InitialChangedBounds = InCreateBootstrapSource
        ? Owner._Bundle._DefaultField._Params.Get_Bounds()
        : FBox{ForceInit};

    auto PublishedEntry = ck_groundnav_world_fields::FEntry{};
    PublishedEntry._VolumeEntity = InOwnerEntity;
    PublishedEntry._Field = MakeShared<const FCk_GroundNav_Field>(Owner._Bundle._DefaultField);
    for (const auto& Variant : Owner._Bundle._VariantFields)
    {
        PublishedEntry._VariantFields.Emplace(
            Variant.Key, MakeShared<const FCk_GroundNav_Field>(Variant.Value));
    }
    ck_groundnav_world_fields::DoApply_PublishNotes(PublishedEntry, FCk_GroundNav_PublishClaim::Geometry());

    ck_groundnav_world_fields::DoBind_CleanupHookOnce();
    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_Write};
    auto* ExistingWorldOwners = ck_groundnav_world_fields::Get_StreamOwners().Find(TWeakObjectPtr<UWorld>{InWorld});
    if (ExistingWorldOwners != nullptr)
    {
        for (const auto& ExistingOwner : *ExistingWorldOwners)
        {
            if (ExistingOwner.Key == InVolumeId || ExistingOwner.Value._OwnerEntity == InOwnerEntity)
            { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::OwnerAlreadyRegistered); }
        }
    }
    auto* ExistingEntries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});
    if (ExistingEntries != nullptr && ck_groundnav_world_fields::DoGet_OwnerEntry(*ExistingEntries, InOwnerEntity) != nullptr)
    { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::OwnerAlreadyRegistered); }

    auto& Entries = ck_groundnav_world_fields::Get_Entries().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});
    auto& WorldOwners = ck_groundnav_world_fields::Get_StreamOwners().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});
    Entries.Emplace(MoveTemp(PublishedEntry));
    WorldOwners.Emplace(InVolumeId, MoveTemp(Owner));
    auto Result = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::Published);
    Result._Source = BootstrapHandle;
    Result._ChangedBounds = InitialChangedBounds;
    Result._Epoch = InitialEpoch;
    return Result;
}

auto ck::groundnav::world_fields::Load_StreamSource(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId,
    TConstArrayView<FCk_GroundNav_StreamTileTransition> InReplacements) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream load requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream load requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    if (InReplacements.IsEmpty()) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange); }

    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    auto Source = ck_groundnav_world_fields::FStreamSource{};
    auto Transitions = TArray<FCk_GroundNav_StreamTileTransition>{InReplacements};
    for (const auto& Transition : Transitions)
    {
        const auto Index = Get_TileIndex(CandidateOwner._Bundle._DefaultField._Params._Divisions, Transition._TileId._Coord);
        auto IsOwned = false;
        for (const auto& ExistingSource : CandidateOwner._Sources)
        { IsOwned |= ExistingSource.Value._Tiles.Contains(Index); }
        if (Transition._Kind != ECk_GroundNav_StreamTileTransitionKind::Replace || IsOwned)
        { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::CoordinateAlreadyOwned); }
        Source._Tiles.Emplace(Index, ck_groundnav_world_fields::FStreamTileState{Transition._TileId._Coord, Transition._DefaultBlob, Transition._VariantBlobs, true});
    }
    Source._Handle = DoAllocate_StreamSourceHandle();
    const auto SourceHandle = Source._Handle;
    CandidateOwner._Sources.Emplace(FCk_GroundNav_StreamRegistryAccess::Get(SourceHandle), MoveTemp(Source));
    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(Transitions), FCk_GroundNav_PublishClaim::Geometry(), Failure);
    if (NOT Prepared.IsSet()) { return Failure; }
    Prepared->_Result._Source = SourceHandle;
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::Replace_StreamSourceTiles(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId, FCk_GroundNav_StreamSourceHandle InSource,
    TConstArrayView<FCk_GroundNav_StreamTileTransition> InReplacements) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream replacement requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream replacement requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    const auto SourceIsValid = InSource.Get_IsValid();
    CK_ENSURE_IF_NOT(SourceIsValid, TEXT("GroundNav stream replacement requires a valid source"))
    {}
    if (NOT SourceIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidSource); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    auto* Source = CandidateOwner._Sources.Find(FCk_GroundNav_StreamRegistryAccess::Get(InSource));
    if (Source == nullptr) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::SourceNotFound); }
    if (InReplacements.IsEmpty()) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange); }
    auto Transitions = TArray<FCk_GroundNav_StreamTileTransition>{InReplacements};
    auto PublishedTransitions = TArray<FCk_GroundNav_StreamTileTransition>{};
    auto SeenTileIndices = TSet<int32>{};
    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    for (const auto& Transition : Transitions)
    {
        const auto TileIdIsValid = Transition._TileId.Get_IsValid() && Transition._TileId._VolumeId == InVolumeId;
        CK_ENSURE_IF_NOT(TileIdIsValid, TEXT("GroundNav stream replacement requires same-volume valid tile ids"))
        {}
        if (NOT TileIdIsValid)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::InvalidTileId;
            return Failure;
        }
        const auto Index = Get_TileIndex(CandidateOwner._Bundle._DefaultField._Params._Divisions, Transition._TileId._Coord);
        if (Transition._Kind != ECk_GroundNav_StreamTileTransitionKind::Replace || NOT Source->_Tiles.Contains(Index))
        { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::CoordinateNotOwned); }
        const auto TileIsUnique = NOT SeenTileIndices.Contains(Index);
        CK_ENSURE_IF_NOT(TileIsUnique, TEXT("GroundNav stream replacement must not contain duplicate tile coordinates"))
        {}
        if (NOT TileIsUnique)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::DuplicateTileCoord;
            return Failure;
        }
        SeenTileIndices.Add(Index);
        const auto& ExistingTile = Source->_Tiles.FindChecked(Index);
        if (NOT ExistingTile._Enabled)
        {
            const auto RetainedReplacementIsValid = DoValidate_RetainedReplacement(
                CandidateOwner._Bundle, InVolumeId, Transition, Failure);
            CK_ENSURE_IF_NOT(RetainedReplacementIsValid, TEXT("GroundNav stream replacement must retain a valid deferred tile"))
            {}
            if (NOT RetainedReplacementIsValid) { return Failure; }
        }
        if (ExistingTile._Enabled)
        { PublishedTransitions.Emplace(Transition); }
    }
    for (const auto& Transition : InReplacements)
    {
        const auto Index = Get_TileIndex(CandidateOwner._Bundle._DefaultField._Params._Divisions, Transition._TileId._Coord);
        const auto WasEnabled = Source->_Tiles.FindChecked(Index)._Enabled;
        Source->_Tiles.FindChecked(Index) = ck_groundnav_world_fields::FStreamTileState{
            Transition._TileId._Coord, Transition._DefaultBlob, Transition._VariantBlobs, WasEnabled};
    }
    if (PublishedTransitions.IsEmpty())
    {
        auto Result = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::Published);
        Result._Source = InSource;
        auto Prepared = DoMake_PreparedStateOnlyTransaction(Snapshot, MoveTemp(CandidateOwner), MoveTemp(Result));
        return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared));
    }
    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(PublishedTransitions), FCk_GroundNav_PublishClaim::Geometry(), Failure);
    if (NOT Prepared.IsSet()) { return Failure; }
    Prepared->_Result._Source = InSource;
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::Upsert_StreamSourceTiles(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId, FCk_GroundNav_StreamSourceHandle InSource,
    TConstArrayView<FCk_GroundNav_StreamTileTransition> InTransitions,
    TConstArrayView<FCk_GroundNav_TileCoord> InEnabledCoords) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream upsert requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream upsert requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    const auto SourceIsValid = InSource.Get_IsValid();
    CK_ENSURE_IF_NOT(SourceIsValid, TEXT("GroundNav stream upsert requires a valid source"))
    {}
    if (NOT SourceIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidSource); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    auto* Source = CandidateOwner._Sources.Find(FCk_GroundNav_StreamRegistryAccess::Get(InSource));
    if (Source == nullptr) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::SourceNotFound); }
    const auto OriginalSource = *Source;

    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    auto SeenTransitionIndices = TSet<int32>{};
    auto TransitionIndices = TArray<int32>{};
    TransitionIndices.Reserve(InTransitions.Num());
    const auto& Divisions = CandidateOwner._Bundle._DefaultField._Params._Divisions;
    for (const auto& Transition : InTransitions)
    {
        const auto TileIdIsValid = Transition._TileId.Get_IsValid() && Transition._TileId._VolumeId == InVolumeId;
        CK_ENSURE_IF_NOT(TileIdIsValid, TEXT("GroundNav stream upsert requires same-volume valid tile ids"))
        {}
        if (NOT TileIdIsValid)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::InvalidTileId;
            return Failure;
        }
        const auto TileIndex = Get_TileIndex(Divisions, Transition._TileId._Coord);
        const auto TileIsInLattice = TileIndex != INDEX_NONE;
        CK_ENSURE_IF_NOT(TileIsInLattice, TEXT("GroundNav stream upsert requires in-lattice tile coordinates"))
        {}
        if (NOT TileIsInLattice)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::TileCoordOutsideLattice;
            return Failure;
        }
        const auto TileIsUnique = NOT SeenTransitionIndices.Contains(TileIndex);
        CK_ENSURE_IF_NOT(TileIsUnique, TEXT("GroundNav stream upsert must not contain duplicate tile coordinates"))
        {}
        if (NOT TileIsUnique)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::DuplicateTileCoord;
            return Failure;
        }
        SeenTransitionIndices.Add(TileIndex);
        TransitionIndices.Emplace(TileIndex);

        const auto SourceOwnsTile = Source->_Tiles.Contains(TileIndex);
        auto OtherSourceOwnsTile = false;
        for (const auto& CandidateSource : CandidateOwner._Sources)
        {
            if (CandidateSource.Key != FCk_GroundNav_StreamRegistryAccess::Get(InSource) &&
                CandidateSource.Value._Tiles.Contains(TileIndex))
            { OtherSourceOwnsTile = true; break; }
        }
        if (Transition._Kind == ECk_GroundNav_StreamTileTransitionKind::Replace)
        {
            if (OtherSourceOwnsTile) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::CoordinateAlreadyOwned); }
            const auto ReplacementIsValid = DoValidate_RetainedReplacement(
                CandidateOwner._Bundle, InVolumeId, Transition, Failure);
            CK_ENSURE_IF_NOT(ReplacementIsValid, TEXT("GroundNav stream upsert requires valid all-profile replacement blobs"))
            {}
            if (NOT ReplacementIsValid) { return Failure; }
            Source->_Tiles.Add(TileIndex, ck_groundnav_world_fields::FStreamTileState{
                Transition._TileId._Coord, Transition._DefaultBlob, Transition._VariantBlobs,
                SourceOwnsTile ? Source->_Tiles.FindChecked(TileIndex)._Enabled : false});
            continue;
        }
        if (Transition._Kind == ECk_GroundNav_StreamTileTransitionKind::Remove)
        {
            const auto RemoveIsValid = Transition._DefaultBlob.IsEmpty() && Transition._VariantBlobs.IsEmpty();
            CK_ENSURE_IF_NOT(RemoveIsValid, TEXT("GroundNav stream upsert remove must not carry tile blobs"))
            {}
            if (NOT RemoveIsValid)
            {
                Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
                Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
                return Failure;
            }
            if (NOT SourceOwnsTile) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::CoordinateNotOwned); }
            Source->_Tiles.Remove(TileIndex);
            continue;
        }

        Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
        Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
        return Failure;
    }

    auto EnabledIndices = TSet<int32>{};
    for (const auto& Coord : InEnabledCoords)
    {
        const auto TileIndex = Get_TileIndex(Divisions, Coord);
        const auto TileIsInLattice = TileIndex != INDEX_NONE;
        CK_ENSURE_IF_NOT(TileIsInLattice, TEXT("GroundNav stream upsert mask requires in-lattice tile coordinates"))
        {}
        if (NOT TileIsInLattice)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::TileCoordOutsideLattice;
            return Failure;
        }
        const auto CoordinateIsUnique = NOT EnabledIndices.Contains(TileIndex);
        CK_ENSURE_IF_NOT(CoordinateIsUnique, TEXT("GroundNav stream upsert mask must not contain duplicate tile coordinates"))
        {}
        if (NOT CoordinateIsUnique)
        {
            Failure._Status = ECk_GroundNav_StreamRegistryStatus::CompositionRefused;
            Failure._CompositionStatus = ECk_GroundNav_StreamCompositionStatus::DuplicateTileCoord;
            return Failure;
        }
        const auto SourceOwnsTile = Source->_Tiles.Contains(TileIndex);
        CK_ENSURE_IF_NOT(SourceOwnsTile, TEXT("GroundNav stream upsert mask may enable only source-owned tiles"))
        {}
        if (NOT SourceOwnsTile) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::CoordinateNotOwned); }
        EnabledIndices.Add(TileIndex);
    }
    for (auto& Tile : Source->_Tiles)
    { Tile.Value._Enabled = EnabledIndices.Contains(Tile.Key); }

    if (DoSourcesMatch(*Source, OriginalSource)) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange); }

    auto AffectedIndices = TSet<int32>{};
    for (const auto& Tile : OriginalSource._Tiles) { AffectedIndices.Add(Tile.Key); }
    for (const auto& Tile : Source->_Tiles) { AffectedIndices.Add(Tile.Key); }
    auto SortedAffectedIndices = AffectedIndices.Array();
    SortedAffectedIndices.Sort();
    auto PublishedTransitions = TArray<FCk_GroundNav_StreamTileTransition>{};
    for (const auto TileIndex : SortedAffectedIndices)
    {
        const auto* OldTile = OriginalSource._Tiles.Find(TileIndex);
        const auto* NewTile = Source->_Tiles.Find(TileIndex);
        if (NewTile != nullptr && NewTile->_Enabled &&
            (OldTile == nullptr || NOT OldTile->_Enabled || NOT DoTilesMatch(*OldTile, *NewTile)))
        {
            PublishedTransitions.Emplace(DoMake_ReplaceTransition(
                InVolumeId, TileIndex, CandidateOwner._Bundle._DefaultField, *NewTile));
        }
        else if (OldTile != nullptr && OldTile->_Enabled && (NewTile == nullptr || NOT NewTile->_Enabled))
        {
            PublishedTransitions.Emplace(ck_groundnav_world_fields::DoMake_RemoveTransition(
                InVolumeId, TileIndex, CandidateOwner._Bundle._DefaultField));
        }
    }

    if (PublishedTransitions.IsEmpty())
    {
        auto Result = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::Published);
        Result._Source = InSource;
        auto Prepared = DoMake_PreparedStateOnlyTransaction(Snapshot, MoveTemp(CandidateOwner), MoveTemp(Result));
        return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared));
    }

    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(PublishedTransitions), FCk_GroundNav_PublishClaim::Geometry(), Failure);
    if (NOT Prepared.IsSet()) { return Failure; }
    Prepared->_Result._Source = InSource;
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::Disable_StreamSource(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId, FCk_GroundNav_StreamSourceHandle InSource) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream disable requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream disable requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    const auto SourceIsValid = InSource.Get_IsValid();
    CK_ENSURE_IF_NOT(SourceIsValid, TEXT("GroundNav stream disable requires a valid source"))
    {}
    if (NOT SourceIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidSource); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    auto* Source = CandidateOwner._Sources.Find(FCk_GroundNav_StreamRegistryAccess::Get(InSource));
    if (Source == nullptr) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::SourceNotFound); }
    auto Transitions = TArray<FCk_GroundNav_StreamTileTransition>{};
    for (const auto& Tile : Source->_Tiles)
    {
        if (Tile.Value._Enabled) { Transitions.Emplace(ck_groundnav_world_fields::DoMake_RemoveTransition(InVolumeId, Tile.Key, CandidateOwner._Bundle._DefaultField)); }
    }
    if (Transitions.IsEmpty()) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange); }
    for (auto& Tile : Source->_Tiles) { Tile.Value._Enabled = false; }
    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(Transitions), FCk_GroundNav_PublishClaim::Geometry(), Failure);
    if (NOT Prepared.IsSet()) { return Failure; }
    Prepared->_Result._Source = InSource;
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::Enable_StreamSource(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId, FCk_GroundNav_StreamSourceHandle InSource) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream enable requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream enable requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    const auto SourceIsValid = InSource.Get_IsValid();
    CK_ENSURE_IF_NOT(SourceIsValid, TEXT("GroundNav stream enable requires a valid source"))
    {}
    if (NOT SourceIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidSource); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    auto* Source = CandidateOwner._Sources.Find(FCk_GroundNav_StreamRegistryAccess::Get(InSource));
    if (Source == nullptr) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::SourceNotFound); }
    auto Transitions = TArray<FCk_GroundNav_StreamTileTransition>{};
    for (const auto& Tile : Source->_Tiles)
    {
        if (NOT Tile.Value._Enabled) { Transitions.Emplace(ck_groundnav_world_fields::DoMake_ReplaceTransition(InVolumeId, Tile.Key, CandidateOwner._Bundle._DefaultField, Tile.Value)); }
    }
    if (Transitions.IsEmpty()) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange); }
    for (auto& Tile : Source->_Tiles) { Tile.Value._Enabled = true; }
    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(Transitions), FCk_GroundNav_PublishClaim::Geometry(), Failure);
    if (NOT Prepared.IsSet()) { return Failure; }
    Prepared->_Result._Source = InSource;
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::Unload_StreamSource(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId, FCk_GroundNav_StreamSourceHandle InSource) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream unload requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream unload requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    const auto SourceIsValid = InSource.Get_IsValid();
    CK_ENSURE_IF_NOT(SourceIsValid, TEXT("GroundNav stream unload requires a valid source"))
    {}
    if (NOT SourceIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidSource); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    auto* Source = CandidateOwner._Sources.Find(FCk_GroundNav_StreamRegistryAccess::Get(InSource));
    if (Source == nullptr) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::SourceNotFound); }
    auto Transitions = TArray<FCk_GroundNav_StreamTileTransition>{};
    auto SourceTileIndices = TArray<int32>{};
    SourceTileIndices.Reserve(Source->_Tiles.Num());
    for (const auto& Tile : Source->_Tiles)
    {
        SourceTileIndices.Emplace(Tile.Key);
        if (Tile.Value._Enabled) { Transitions.Emplace(ck_groundnav_world_fields::DoMake_RemoveTransition(InVolumeId, Tile.Key, CandidateOwner._Bundle._DefaultField)); }
    }
    CandidateOwner._Sources.Remove(FCk_GroundNav_StreamRegistryAccess::Get(InSource));
    if (Transitions.IsEmpty())
    {
        const auto ChangedBounds = ck_groundnav_world_fields::DoGet_ChangedBounds(
            CandidateOwner._Bundle._DefaultField, SourceTileIndices);
        auto Prepared = DoMake_PreparedPublishTransaction(
            Snapshot, MoveTemp(CandidateOwner), FCk_GroundNav_PublishClaim::Geometry(),
            ChangedBounds, SourceTileIndices);
        Prepared._Result._Source = InSource;
        return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared));
    }

    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(Transitions), FCk_GroundNav_PublishClaim::Geometry(), Failure);
    if (NOT Prepared.IsSet()) { return Failure; }
    Prepared->_Result._Source = InSource;
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::Unregister_StreamOwner(
    UWorld* InWorld, const FCk_Handle& InOwnerEntity, FCk_GroundNav_VolumeId InVolumeId) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream owner teardown requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto OwnerIsValid = ck::IsValid(InOwnerEntity);
    CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("GroundNav stream owner teardown requires a valid owner"))
    {}
    if (NOT OwnerIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidOwner); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream owner teardown requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_Write};
    auto* Owner = DoFind_StreamOwner(InWorld, InVolumeId);
    if (Owner == nullptr || Owner->_OwnerEntity != InOwnerEntity) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::OwnerNotRegistered); }
    auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});
    auto* Entry = Entries == nullptr ? nullptr : ck_groundnav_world_fields::DoGet_OwnerEntry(*Entries, InOwnerEntity);
    if (Entry == nullptr) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::OwnerEntryMissing); }
    auto Result = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::Published);
    Result._ChangedBounds = Entry->_Field.IsValid() ? Entry->_Field->_Params.Get_Bounds() : FBox{ForceInit};
    Result._Epoch = Entry->_Field.IsValid() ? Entry->_Field->_Epoch.Get_Next() : FCk_GroundNav_Epoch{};
    auto& RetiredRevision = ck_groundnav_world_fields::Get_RetiredRevisions().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});
    Entries->RemoveAll([&](const ck_groundnav_world_fields::FEntry& Candidate) -> bool
    {
        if (Candidate._VolumeEntity != InOwnerEntity) { return false; }
        if (Candidate._Field.IsValid()) { RetiredRevision += Candidate._Field->Get_AggregatedTileEpochSum(); }
        for (const auto& Variant : Candidate._VariantFields)
        { if (Variant.Value.IsValid()) { RetiredRevision += Variant.Value->Get_AggregatedTileEpochSum(); } }
        return true;
    });
    ck_groundnav_world_fields::Get_StreamOwners().FindChecked(TWeakObjectPtr<UWorld>{InWorld}).Remove(InVolumeId);
    return Result;
}

auto ck::groundnav::world_fields::Refresh_StreamOwnerBundle(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId, const FCk_GroundNav_StreamFieldBundle& InRefreshedBundle,
    const FCk_GroundNav_PublishClaim& InClaim) -> FCk_GroundNav_StreamRegistryResult
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream refresh requires a valid world"))
    {}
    if (NOT WorldIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidWorld); }
    const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
    CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream refresh requires a positive volume id"))
    {}
    if (NOT VolumeIdIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidVolumeId); }
    const auto BundleIsValid = ck_groundnav_world_fields::DoValidate_Bundle(InRefreshedBundle);
    CK_ENSURE_IF_NOT(BundleIsValid, TEXT("GroundNav stream refresh requires a valid all-profile bundle"))
    {}
    if (NOT BundleIsValid) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidBundle); }

    auto Snapshot = FStreamOwnerTransactionSnapshot{};
    const auto SnapshotStatus = DoCopy_StreamOwnerTransactionSnapshot(InWorld, InVolumeId, Snapshot);
    if (SnapshotStatus != ECk_GroundNav_StreamRegistryStatus::Published) { return DoMake_Result(SnapshotStatus); }
    auto CandidateOwner = DoMake_CandidateOwner(Snapshot);
    const auto BundlesMatchCanonical = ck_groundnav_world_fields::DoBundlesMatchCanonical(
        InRefreshedBundle, CandidateOwner._Bundle);
    CK_ENSURE_IF_NOT(BundlesMatchCanonical,
        TEXT("GroundNav stream refresh must retain the registered lattice and profiles"))
    {}
    if (NOT BundlesMatchCanonical) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidBundle); }

    // Stage every retained blob from the complete derived bundle first. Disabled sources participate
    // in this validation/serialization pass even though their tiles are removed from publication.
    auto Candidate = InRefreshedBundle;
    auto EnabledTileIndices = TSet<int32>{};
    for (const auto& Source : CandidateOwner._Sources)
    {
        for (const auto& Tile : Source.Value._Tiles)
        {
            const auto IsBuiltEverywhere = Candidate._DefaultField._Tiles.IsValidIndex(Tile.Key) &&
                Candidate._DefaultField._Tiles[Tile.Key].Get_IsBuilt() &&
                Candidate._DefaultField._Tiles[Tile.Key]._Coord == Tile.Value._Coord;
            CK_ENSURE_IF_NOT(IsBuiltEverywhere,
                TEXT("GroundNav stream refresh must provide every retained default tile"))
            {}
            if (NOT IsBuiltEverywhere) { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidBundle); }
            for (const auto& Variant : Candidate._VariantFields)
            {
                const auto VariantHasTile = Variant.Value._Tiles.IsValidIndex(Tile.Key) &&
                    Variant.Value._Tiles[Tile.Key].Get_IsBuilt() &&
                    Variant.Value._Tiles[Tile.Key]._Coord == Tile.Value._Coord;
                CK_ENSURE_IF_NOT(VariantHasTile,
                    TEXT("GroundNav stream refresh must provide every retained variant tile"))
                {}
                if (NOT VariantHasTile)
                { return DoMake_Result(ECk_GroundNav_StreamRegistryStatus::InvalidBundle); }
            }
        }
    }
    for (auto& Source : CandidateOwner._Sources)
    {
        for (auto& Tile : Source.Value._Tiles)
        {
            const auto Coord = Get_TileCoord(Candidate._DefaultField._Params._Divisions, Tile.Key);
            // Write_Tile serializes Candidate into the retained blob. This must run even for a
            // disabled source: Enable_StreamSource later republishes these blobs without probing.
            Write_Tile(Candidate._DefaultField, Coord, Tile.Value._DefaultBlob);
            for (const auto& Variant : Candidate._VariantFields)
            { Write_Tile(Variant.Value, Coord, Tile.Value._VariantBlobs.FindOrAdd(Variant.Key)); }
            if (Tile.Value._Enabled)
            { EnabledTileIndices.Add(Tile.Key); }
        }
    }

    auto RemovalTransitions = TArray<FCk_GroundNav_StreamTileTransition>{};
    for (auto TileIndex = 0; TileIndex < Candidate._DefaultField._Tiles.Num(); ++TileIndex)
    {
        if (Candidate._DefaultField._Tiles[TileIndex].Get_IsBuilt() && NOT EnabledTileIndices.Contains(TileIndex))
        { RemovalTransitions.Emplace(ck_groundnav_world_fields::DoMake_RemoveTransition(InVolumeId, TileIndex, Candidate._DefaultField)); }
    }

    CandidateOwner._Bundle = MoveTemp(Candidate);
    const auto RefreshedTileIndices = DoMake_AllTileIndices(CandidateOwner._Bundle);
    if (RemovalTransitions.IsEmpty())
    {
        auto Prepared = DoMake_PreparedPublishTransaction(
            Snapshot, MoveTemp(CandidateOwner), InClaim,
            InRefreshedBundle._DefaultField._Params.Get_Bounds(), RefreshedTileIndices);
        return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared));
    }

    auto Failure = DoMake_Result(ECk_GroundNav_StreamRegistryStatus::NoChange);
    auto Prepared = DoCompose_PreparePublish(
        Snapshot, MoveTemp(CandidateOwner), MoveTemp(RemovalTransitions), InClaim, Failure, &RefreshedTileIndices);
    if (NOT Prepared.IsSet()) { return Failure; }
    return DoCommit_PreparedStreamOwnerTransaction(InWorld, InVolumeId, MoveTemp(Prepared.GetValue()));
}

auto ck::groundnav::world_fields::TryGet_StreamOwnerSnapshot(
    UWorld* InWorld, FCk_GroundNav_VolumeId InVolumeId) -> TOptional<FCk_GroundNav_StreamOwnerSnapshot>
{
    if (ck::Is_NOT_Valid(InWorld)) { return {}; }
    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_ReadOnly};
    const auto* Owner = DoFind_StreamOwner(InWorld, InVolumeId);
    if (Owner == nullptr) { return {}; }
    const auto* Entries = ck_groundnav_world_fields::Get_Entries().Find(TWeakObjectPtr<UWorld>{InWorld});
    const auto* Entry = static_cast<const ck_groundnav_world_fields::FEntry*>(nullptr);
    if (Entries != nullptr)
    {
        for (const auto& Candidate : *Entries)
        {
            if (Candidate._VolumeEntity == Owner->_OwnerEntity)
            { Entry = &Candidate; break; }
        }
    }
    if (Entry == nullptr) { return {}; }
    auto Snapshot = FCk_GroundNav_StreamOwnerSnapshot{};
    Snapshot._DefaultField = Entry->_Field;
    Snapshot._VariantFields = Entry->_VariantFields;
    Snapshot._DefaultPublishNote = Entry->_PublishNote;
    Snapshot._VariantPublishNotes = Entry->_VariantPublishNotes;
    Snapshot._Epoch = Entry->_Field.IsValid() ? Entry->_Field->_Epoch : FCk_GroundNav_Epoch{};
    return Snapshot;
}
