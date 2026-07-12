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
    UPROPERTY() FCk_Snapshot_Header _Header;
    UPROPERTY() TArray<uint8>       _SnapshotBytes;

    // Format v3 (rebuild+hydrate) written alongside Model A until Phase 5. Loading still consumes _SnapshotBytes
    // (Model A) until Phase 3B lands; v3 is inspectable now.
    UPROPERTY() FCk_Snapshot_HeaderV3 _HeaderV3;
    UPROPERTY() TArray<uint8>         _SnapshotBytesV3;
};
