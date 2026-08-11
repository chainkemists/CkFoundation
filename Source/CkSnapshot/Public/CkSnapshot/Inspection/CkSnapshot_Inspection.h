#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/Inspection/CkSnapshot_Inspection_Data.h"

class UCk_Snapshot_SaveGame;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    /** Offline inspection of a CK save. Nothing here instantiates a saved entity, touches a registry, opens a map
     *  or fabricates a live handle — every entity reference produced is a raw saved id.
     *
     *  ALL entry points are GAME-THREAD ONLY: the envelope route creates a USaveGame UObject and the decode route
     *  may resolve (and, under AllowTypeLoad, load) a UScriptStruct.
     *
     *  A malformed, foreign or stale file is expected input and reports a status — it never fires an ensure. */

    /** Absent file -> FileNotFound; unreadable existing file -> IoFailure; otherwise the byte route. */
    CKSNAPSHOT_API auto
    Inspect_SaveFile(
        const FString& InAbsolutePath) -> FCk_SnapshotInspection_Document;

    /** Slot of the same user index the subsystem saves under. Absent slot -> FileNotFound. */
    CKSNAPSHOT_API auto
    Inspect_SaveSlot(
        FName InSlotName) -> FCk_SnapshotInspection_Document;

    /** Raw bytes of an engine save envelope. InSourceDescription is carried into the document verbatim. */
    CKSNAPSHOT_API auto
    Inspect_SaveBytes(
        const TArray<uint8>& InBytes,
        const FString& InSourceDescription) -> FCk_SnapshotInspection_Document;

    /** Envelope-free seam: inspects an already-materialized save object. Source byte count and hash stay unset. */
    CKSNAPSHOT_API auto
    Inspect_SaveGameObject(
        UCk_Snapshot_SaveGame* InSaveGame,
        const FString& InSourceDescription) -> FCk_SnapshotInspection_Document;

    // ----------------------------------------------------------------------------------------------------------------

    /** Decodes one payload row's blob into a bounded value tree. Handle-typed fields land as SavedEntityRef leaves
     *  carrying the raw saved id — the projection runs against a default snapshot context, so no registry is
     *  consulted and no handle is ever re-homed onto a live world. */
    CKSNAPSHOT_API auto
    TryDecode_Payload(
        const FCk_SnapshotInspection_Document& InDocument,
        int32 InPayloadIndex,
        ECk_SnapshotInspection_DecodePolicy InPolicy) -> FCk_SnapshotInspection_DecodeResult;

    /** Same, for one entity row's RuntimeSpawned spawn-params blob. Spawn params declare no type path, so the
     *  DeclaredTypeMismatch outcome cannot arise here. */
    CKSNAPSHOT_API auto
    TryDecode_SpawnParams(
        const FCk_SnapshotInspection_Document& InDocument,
        int32 InEntityIndex,
        ECk_SnapshotInspection_DecodePolicy InPolicy) -> FCk_SnapshotInspection_DecodeResult;

    // ----------------------------------------------------------------------------------------------------------------

    // Shared rendering: the console census, the load-time skip/orphan records and any future inspector all read a
    // save row through these two, so their vocabulary cannot drift apart.

    CKSNAPSHOT_API auto
    Get_ProvenanceText(
        ECk_Snapshot_V3_Provenance InProvenance) -> const TCHAR*;

    /** The identity a row is addressed BY, per provenance — the only human-readable key available once a saved
     *  id has failed to resolve. */
    CKSNAPSHOT_API auto
    Get_IdentityText(
        const FCk_Snapshot_V3_EntityEntry& InEntry) -> FString;

    CKSNAPSHOT_API auto
    Get_ReadStatusText(
        ECk_SnapshotInspection_ReadStatus InStatus) -> const TCHAR*;

    CKSNAPSHOT_API auto
    Get_CompatibilityText(
        ECk_SnapshotInspection_Compatibility InCompatibility) -> const TCHAR*;

    CKSNAPSHOT_API auto
    Get_DecodeStatusText(
        ECk_SnapshotInspection_DecodeStatus InStatus) -> const TCHAR*;

    CKSNAPSHOT_API auto
    Get_SeverityText(
        ECk_SnapshotInspection_Severity InSeverity) -> const TCHAR*;

    CKSNAPSHOT_API auto
    Get_DiagnosticCodeText(
        ECk_SnapshotInspection_DiagnosticCode InCode) -> const TCHAR*;

    CKSNAPSHOT_API auto
    Get_ValueKindText(
        ECk_SnapshotInspection_ValueKind InKind) -> const TCHAR*;
}

// --------------------------------------------------------------------------------------------------------------------
