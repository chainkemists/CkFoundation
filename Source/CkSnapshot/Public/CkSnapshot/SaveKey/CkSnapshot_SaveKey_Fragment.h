#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_SaveKey_Fragment.generated.h"

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FFragment_SaveKey
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_SaveKey);
    using IsSnapshotable = void;

private:
    UPROPERTY(meta=(SaveGame))
    FGuid _Key;

public:
    CK_PROPERTY_GET(_Key);
    CK_DEFINE_CONSTRUCTORS(FFragment_SaveKey, _Key);
};
