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
    // The leading dotted-numeric run only. Anything from the first non-digit, non-dot character onward is a
    // label ("-hotfix1", "-rc2") that carries no order. An empty result means "no numeric lead at all".
    auto
        DoParse_VersionSegments(
            const FString& InVersion)
        -> TArray<int64>
    {
        auto Segments = TArray<int64>{};
        auto Current  = int64{0};
        auto HasDigits = false;

        for (const auto Char : InVersion)
        {
            if (FChar::IsDigit(Char))
            {
                // Saturate rather than overflow on an absurd segment: it stays "very new" instead of wrapping
                // to a small number and reading as ancient.
                Current = FMath::Min(Current * 10 + (Char - TCHAR('0')), TNumericLimits<int64>::Max() / 16);
                HasDigits = true;
                continue;
            }

            if (Char == TCHAR('.') && HasDigits)
            {
                Segments.Add(Current);
                Current   = 0;
                HasDigits = false;
                continue;
            }

            break;
        }

        if (HasDigits)
        { Segments.Add(Current); }

        return Segments;
    }
}

auto
    ck::snapshot::
    Compare_ProjectVersions(
        const FString& InA,
        const FString& InB)
    -> int32
{
    const auto SegmentsA = ck_snapshot_header::DoParse_VersionSegments(InA);
    const auto SegmentsB = ck_snapshot_header::DoParse_VersionSegments(InB);

    // Unparseable is OLDER than anything parseable - an unstamped save must land on the compensating side of a
    // gate - and two unparseable versions are indistinguishable rather than ordered.
    if (SegmentsA.IsEmpty() || SegmentsB.IsEmpty())
    { return SegmentsA.IsEmpty() && SegmentsB.IsEmpty() ? 0 : (SegmentsA.IsEmpty() ? -1 : 1); }

    const auto SegmentCount = FMath::Max(SegmentsA.Num(), SegmentsB.Num());
    for (auto Index = 0; Index < SegmentCount; ++Index)
    {
        // A missing trailing segment is zero, so "1.0" and "1.0.0" are the same version.
        const auto ValueA = SegmentsA.IsValidIndex(Index) ? SegmentsA[Index] : 0;
        const auto ValueB = SegmentsB.IsValidIndex(Index) ? SegmentsB[Index] : 0;

        if (ValueA != ValueB)
        { return ValueA < ValueB ? -1 : 1; }
    }

    return 0;
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
