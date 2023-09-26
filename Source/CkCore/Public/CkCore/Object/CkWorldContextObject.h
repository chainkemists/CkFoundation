#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkWorldContextObject.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

UCLASS(BlueprintType)
class CKCORE_API UCk_GameWorldContextObject_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameWorldContextObject_UE);

public:
    auto GetWorld() const -> UWorld* override;
};

// --------------------------------------------------------------------------------------------------------------------
