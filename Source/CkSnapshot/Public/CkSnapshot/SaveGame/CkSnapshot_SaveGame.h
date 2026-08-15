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

    UPROPERTY()
    TArray<uint8> _SnapshotBytesV3;
};
