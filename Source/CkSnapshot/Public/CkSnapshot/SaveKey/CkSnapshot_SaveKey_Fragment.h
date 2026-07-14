#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_SaveKey_Fragment.generated.h"

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FFragment_SaveKey
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_SaveKey);

private:
    // UPROPERTY(SaveGame) sets the real CPF_SaveGame flag so Tier-A reflection serializes _Key under an
    // ArIsSaveGame archive. The former UPROPERTY(meta=(SaveGame)) only stuffed "SaveGame" into the metadata
    // map (not the flag), so the GUID round-tripped empty — see FFragment_ActorSpawnIntent.h:29-32.
    UPROPERTY(SaveGame)
    FGuid _Key;

public:
    CK_PROPERTY_GET(_Key);
    CK_DEFINE_CONSTRUCTORS(FFragment_SaveKey, _Key);
};
