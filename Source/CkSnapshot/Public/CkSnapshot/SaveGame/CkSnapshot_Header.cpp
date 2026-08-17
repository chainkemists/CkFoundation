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

    // Fail closed rather than hand a corrupt count to SetNumUninitialized.
    if (Count < 0)
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
    template <typename T_Entry>
    auto
        DoSerialize_EntryArray(
            FArchive& InAr,
            TArray<T_Entry>& InOutEntries)
        -> void
    {
        auto Count = InOutEntries.Num();
        InAr << Count;

        if (Count < 0)
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

    ck_snapshot_header::DoSerialize_EntryArray(InAr, _BuildRecipe);

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
    ck_snapshot_header::DoSerialize_EntryArray(InAr, _Entities);
    ck_snapshot_header::DoSerialize_EntryArray(InAr, _Payloads);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
