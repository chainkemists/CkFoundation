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
};
