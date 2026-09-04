#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>
#include <Misc/ScopeRWLock.h>
#include <UObject/WeakObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_world_fields
{
    struct FEntry
    {
        FCk_Handle _VolumeEntity;
        ck::groundnav::FCk_GroundNav_FieldPtr _Field;

        // Beside the pointer it accounts for and updated with it, so the two can never describe
        // different publishes for as long as a reader holds the lock.
        ck::groundnav::world_fields::FCk_GroundNav_PublishNote _PublishNote;
    };

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

    // The lock guards the POINTER HANDOFF and nothing else. A reader copies a shared pointer out and
    // then queries the field it names with no lock held: the field is immutable, so the only thing two
    // threads can disagree about is which pointer is current, and that is exactly what this covers.
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
     * several of them still answer by identity rather than falling back to bounds.
     */
    auto DoApply_Publish(
        ck::groundnav::world_fields::FCk_GroundNav_PublishNote& InOutNote,
        const ck::groundnav::FCk_GroundNav_Epoch&               InPublishedEpoch,
        const TOptional<TArray<int32>>&                         InLinkOnlyChangedLinkIds) -> void
    {
        InOutNote._Epoch = InPublishedEpoch;

        if (NOT InLinkOnlyChangedLinkIds.IsSet())
        {
            InOutNote._LastGeometryEpoch = InPublishedEpoch;
            InOutNote._ChangedLinkIdsSinceGeometry.Reset();

            return;
        }

        for (const auto ChangedLinkId : *InLinkOnlyChangedLinkIds)
        { InOutNote._ChangedLinkIdsSinceGeometry.AddUnique(ChangedLinkId); }

        InOutNote._ChangedLinkIdsSinceGeometry.Sort();
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
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Publish(
        UWorld*                         InWorld,
        const FCk_Handle&               InVolumeEntity,
        FCk_GroundNav_FieldPtr          InField,
        const TOptional<TArray<int32>>& InLinkOnlyChangedLinkIds)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    ck_groundnav_world_fields::DoBind_CleanupHookOnce();

    // Read before the pointer is moved out, and taken from the FIELD rather than from the caller: the
    // note's epochs are what a reader compares its own snapshot against, so they can only be the ones
    // the thing being published actually carries. A volume registering with no field yet has none.
    const auto PublishedEpoch = InField.IsValid() ? InField->_Epoch : FCk_GroundNav_Epoch{};

    auto Lock = FRWScopeLock{ck_groundnav_world_fields::Get_Lock(), SLT_Write};

    auto& Entries = ck_groundnav_world_fields::Get_Entries().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});

    for (auto& Entry : Entries)
    {
        if (Entry._VolumeEntity != InVolumeEntity)
        { continue; }

        Entry._Field = MoveTemp(InField);

        ck_groundnav_world_fields::DoApply_Publish(
            Entry._PublishNote, PublishedEpoch, InLinkOnlyChangedLinkIds);

        return;
    }

    auto NewEntry = ck_groundnav_world_fields::FEntry{InVolumeEntity, MoveTemp(InField), {}};

    ck_groundnav_world_fields::DoApply_Publish(
        NewEntry._PublishNote, PublishedEpoch, InLinkOnlyChangedLinkIds);

    Entries.Emplace(MoveTemp(NewEntry));
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

    if (Entries == nullptr)
    { return; }

    auto& RetiredRevision = ck_groundnav_world_fields::Get_RetiredRevisions().FindOrAdd(TWeakObjectPtr<UWorld>{InWorld});

    Entries->RemoveAll([&](const ck_groundnav_world_fields::FEntry& InEntry) -> bool
    {
        if (InEntry._VolumeEntity != InVolumeEntity)
        { return false; }

        if (InEntry._Field.IsValid())
        { RetiredRevision += InEntry._Field->Get_AggregatedTileEpochSum(); }

        return true;
    });
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
    TryGet_PublishNote(
        UWorld*        InWorld,
        const FVector& InLocation)
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

    // Copied out under the lock, exactly as the pointer beside it is, so what the caller reads is a
    // value nothing can change under it.
    return Entry->_PublishNote;
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

// --------------------------------------------------------------------------------------------------------------------
