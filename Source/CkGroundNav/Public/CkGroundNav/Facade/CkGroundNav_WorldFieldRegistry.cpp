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
    };

    using FWorldEntries = TMap<TWeakObjectPtr<UWorld>, TArray<FEntry>>;

    auto Get_Entries() -> FWorldEntries&
    {
        static auto Entries = FWorldEntries{};
        return Entries;
    }

    // The lock guards the POINTER HANDOFF and nothing else. A reader copies a shared pointer out and
    // then queries the field it names with no lock held: the field is immutable, so the only thing two
    // threads can disagree about is which pointer is current, and that is exactly what this covers.
    auto Get_Lock() -> FRWLock&
    {
        static auto Lock = FRWLock{};
        return Lock;
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
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::groundnav::world_fields::
    Publish(
        UWorld*                InWorld,
        const FCk_Handle&      InVolumeEntity,
        FCk_GroundNav_FieldPtr InField)
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

        Entry._Field = MoveTemp(InField);
        return;
    }

    Entries.Emplace(ck_groundnav_world_fields::FEntry{InVolumeEntity, MoveTemp(InField)});
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

    auto FirstField = FCk_GroundNav_FieldPtr{};

    for (const auto& Entry : *Entries)
    {
        if (NOT Entry._Field.IsValid())
        { continue; }

        if (NOT FirstField.IsValid())
        { FirstField = Entry._Field; }

        if (Entry._Field->_Params.Get_Bounds().IsInsideOrOn(InLocation))
        { return Entry._Field; }
    }

    return FirstField;
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
