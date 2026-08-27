#include "CkSnapshot_SaveGame.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Kismet/GameplayStatics.h"
#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_SaveGame::
    Serialize(
        FArchive& InAr)
    -> void
{
    Super::Serialize(InAr);

    ck::snapshot::Serialize_BulkBytes(InAr, _SnapshotBytesV3);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_save_game
    {
        // FSaveGameHeader::FileTypeTag — see Get_HasSaveGameEnvelopeTag's header comment.
        constexpr auto k_SaveGameFileTypeTag = 0x53415647;
    }

    auto
        Get_HasSaveGameEnvelopeTag(
            const TArray<uint8>& InBytes)
        -> bool
    {
        if (InBytes.Num() < 4)
        { return false; }

        const auto Tag = static_cast<int32>(
              static_cast<uint32>(InBytes[0])
            | (static_cast<uint32>(InBytes[1]) << 8)
            | (static_cast<uint32>(InBytes[2]) << 16)
            | (static_cast<uint32>(InBytes[3]) << 24));

        return Tag == ck_snapshot_save_game::k_SaveGameFileTypeTag;
    }

    auto
        TryLoad_SlotSaveGame_Guarded(
            const FString& InSlotName,
            int32 InUserIndex)
        -> USaveGame*
    {
        auto Bytes = TArray<uint8>{};
        if (NOT UGameplayStatics::LoadDataFromSlot(Bytes, InSlotName, InUserIndex))
        { return nullptr; }

        if (NOT Get_HasSaveGameEnvelopeTag(Bytes))
        { return nullptr; }

        return UGameplayStatics::LoadGameFromMemory(Bytes);
    }

    auto
        Get_HasCompatiblePayload(
            const UCk_Snapshot_SaveGame& InSaveGame)
        -> bool
    {
        return InSaveGame._HeaderV3.Get_FormatVersion() == FCk_Snapshot_HeaderV3::CurrentFormatVersion &&
               NOT InSaveGame._SnapshotBytesV3.IsEmpty();
    }

    auto
        Get_SlotHoldsCompatibleSave(
            FName InSlotName,
            int32 InUserIndex)
        -> bool
    {
        auto* SaveGame = Cast<UCk_Snapshot_SaveGame>(
            TryLoad_SlotSaveGame_Guarded(InSlotName.ToString(), InUserIndex));

        return ck::IsValid(SaveGame) && Get_HasCompatiblePayload(*SaveGame);
    }
}

// --------------------------------------------------------------------------------------------------------------------
