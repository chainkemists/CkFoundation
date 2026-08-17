#include "CkSnapshot_Header.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::snapshot::
    Serialize_BulkBytes(
        FArchive& InAr,
        TArray<uint8>& InOutBytes)
    -> void
{
    auto Count = InOutBytes.Num();
    InAr << Count;

    // Fail closed rather than hand a corrupt count to SetNumUninitialized — negative, or claiming more
    // bytes than the archive still holds (which would otherwise allocate gigabytes before running dry).
    // TotalSize is INDEX_NONE on archives with no known backing size; only the known-size check is skipped there.
    const auto TotalSize = InAr.TotalSize();
    const auto CountIsCorrupt = Count < 0 ||
        (InAr.IsLoading() && TotalSize >= 0 && Count > TotalSize - InAr.Tell());
    if (CountIsCorrupt)
    {
        InAr.SetError();
        return;
    }

    if (InAr.IsLoading())
    { InOutBytes.SetNumUninitialized(Count); }

    if (Count > 0)
    { InAr.Serialize(InOutBytes.GetData(), Count); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::snapshot::
    Serialize_Transform(
        FArchive& InAr,
        FTransform& InOutTransform)
    -> void
{
    auto Rotation    = InOutTransform.GetRotation();
    auto Translation = InOutTransform.GetTranslation();
    auto Scale       = InOutTransform.GetScale3D();

    InAr << Rotation;
    InAr << Translation;
    InAr << Scale;

    if (InAr.IsLoading())
    { InOutTransform = FTransform{Rotation, Translation, Scale}; }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_snapshot_header
{
    // TArray<T>'s operator<< needs an `Ar << Element` for T, which a WithSerializer USTRUCT does not get.
    // Explicit count + per-element native Serialize keeps every nested array on the native path.
    // InMinWireBytesPerEntry is a conservative floor of one EMPTY entry's stream, so a corrupt count
    // claiming more entries than the remaining archive could possibly hold fails closed instead of
    // allocating gigabytes.
    template <typename T_Entry>
    auto
        DoSerialize_EntryArray(
            FArchive& InAr,
            TArray<T_Entry>& InOutEntries,
            int32 InMinWireBytesPerEntry)
        -> void
    {
        auto Count = InOutEntries.Num();
        InAr << Count;

        const auto TotalSize = InAr.TotalSize();
        const auto CountIsCorrupt = Count < 0 ||
            (InAr.IsLoading() && TotalSize >= 0 &&
             static_cast<int64>(Count) * InMinWireBytesPerEntry > TotalSize - InAr.Tell());
        if (CountIsCorrupt)
        {
            InAr.SetError();
            return;
        }

        if (InAr.IsLoading())
        { InOutEntries.SetNum(Count); }

        for (auto& Entry : InOutEntries)
        { Entry.Serialize(InAr); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_V3_BuildStep::
    Serialize(
        FArchive& InAr)
    -> bool
{
    InAr << _ScriptClassPath;
    InAr << _ArchetypePath;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_V3_EntityEntry::
    Serialize(
        FArchive& InAr)
    -> bool
{
    InAr << _SavedId;

    auto ProvenanceByte = static_cast<uint8>(_Provenance);
    InAr << ProvenanceByte;
    if (InAr.IsLoading())
    { _Provenance = static_cast<ECk_Snapshot_V3_Provenance>(ProvenanceByte); }

    InAr << _LifetimeOwnerSavedId;
    InAr << _SaveKey;
    InAr << _PlayerId;
    InAr << _Label;
    InAr << _ScriptClassPath;

    ck::snapshot::Serialize_BulkBytes(InAr, _SpawnParamsBytes);

    InAr << _ContextOwnerSavedId;
    InAr << _ActorClassPath;

    ck::snapshot::Serialize_BulkBytes(InAr, _ActorSaveFieldBytes);

    ck::snapshot::Serialize_Transform(InAr, _ActorSpawnTransform);
    ck::snapshot::Serialize_Transform(InAr, _SavedWorldTransform);

    constexpr auto MinWireBytesPerBuildStep = 8; // two empty FStrings
    ck_snapshot_header::DoSerialize_EntryArray(InAr, _BuildRecipe, MinWireBytesPerBuildStep);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_V3_PayloadEntry::
    Serialize(
        FArchive& InAr)
    -> bool
{
    InAr << _OwnerSavedId;
    InAr << _TypePath;
    ck::snapshot::Serialize_BulkBytes(InAr, _PayloadBytes);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_V3_Tables::
    Serialize(
        FArchive& InAr)
    -> bool
{
    constexpr auto MinWireBytesPerEntity  = 64; // ids + guid + provenance + empty strings/blobs + two transforms
    constexpr auto MinWireBytesPerPayload = 12; // owner id + empty type path + empty blob count
    ck_snapshot_header::DoSerialize_EntryArray(InAr, _Entities, MinWireBytesPerEntity);
    ck_snapshot_header::DoSerialize_EntryArray(InAr, _Payloads, MinWireBytesPerPayload);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
