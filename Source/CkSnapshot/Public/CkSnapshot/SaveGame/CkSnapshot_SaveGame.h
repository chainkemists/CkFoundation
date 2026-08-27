#pragma once

#include "GameFramework/SaveGame.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_Header.h"

#include "CkSnapshot_SaveGame.generated.h"

UCLASS()
class CKSNAPSHOT_API UCk_Snapshot_SaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Snapshot_SaveGame);

public:
    // Format v3 (rebuild+hydrate, spec §4.2) is the SOLE save format since Phase 5 (Model A decommissioned). The
    // Model-A _Header / _SnapshotBytes fields were removed; Get_SaveSlotHeader now synthesizes its legacy-shaped
    // FCk_Snapshot_Header return value from _HeaderV3 at read time.
    UPROPERTY()
    FCk_Snapshot_HeaderV3 _HeaderV3;

    // Not a UPROPERTY on purpose — see ck::snapshot::Serialize_BulkBytes, which Serialize() below routes it through.
    TArray<uint8> _SnapshotBytesV3;

public:
    virtual auto Serialize(FArchive& InAr) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    /** True when the bytes begin with FSaveGameHeader::FileTypeTag ('GVAS'), written first as an int32 by every
     *  engine save envelope. The engine's own UE_SAVEGAME_FILE_TYPE_TAG lives in GameplayStatics.cpp as a
     *  file-local static exported by no public header, so it is named in this module rather than left bare at the
     *  comparison sites. */
    CKSNAPSHOT_API auto
    Get_HasSaveGameEnvelopeTag(
        const TArray<uint8>& InBytes) -> bool;

    /** The ONLY sanctioned way to materialize a save slot's bytes as a USaveGame.
     *
     *  UGameplayStatics::LoadGameFromSlot / LoadGameFromMemory must never see un-tagged bytes: their legacy
     *  no-tag fallback rewinds and parses the RAW bytes as a headerless v1 save, and on a foreign file (a
     *  SPUD-era save squatting on a slot name) that misparse is FATAL, not an error return — measured
     *  2026-08-27 by the ForeignFile spec: hard assertion in FName loading (UnrealNames.cpp:3252), process
     *  death. This loader reads the raw bytes, refuses anything without the envelope tag, and only then hands
     *  them to the engine. Null means absent, unreadable, or foreign. */
    CKSNAPSHOT_API auto
    TryLoad_SlotSaveGame_Guarded(
        const FString& InSlotName,
        int32 InUserIndex) -> USaveGame*;

    /** The payload half of Request_Load's acceptance gate, shared with the slot-occupancy probe below so the two
     *  cannot drift: the envelope-loaded object carries the CURRENT v3 format version and a non-empty payload
     *  stream. Deliberately does NOT parse the payload tables — that stays Request_Load's own pre-teardown step, so
     *  a corrupt-tables save remains visible to a menu (hiding a damaged real save would read as "my save
     *  vanished") and is refused with Failed_Corrupt on the actual load. */
    CKSNAPSHOT_API auto
    Get_HasCompatiblePayload(
        const UCk_Snapshot_SaveGame& InSaveGame) -> bool;

    /** Slot-occupancy gate for menus: the slot's file exists, its envelope deserializes as UCk_Snapshot_SaveGame,
     *  and Get_HasCompatiblePayload accepts it. A foreign or stale file — a legacy SPUD-era save, another game's
     *  USaveGame, an older CkSnapshot format — is EXPECTED input and answers false without ensuring. */
    CKSNAPSHOT_API auto
    Get_SlotHoldsCompatibleSave(
        FName InSlotName,
        int32 InUserIndex) -> bool;
}
