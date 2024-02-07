#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DataAsset.h>
#include <CoreMinimal.h>

#include "CkDataAsset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, EditInlineNew)
class CKCORE_API UCk_DataAsset_PDA : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DataAsset_PDA);

public:
    auto GetWorld() const -> UWorld* override;

private:
    static auto Get_WorldFromOuterRecursively(UObject* InObject) -> UWorld*;

protected:
    mutable TWeakObjectPtr<UWorld> _CurrentWorld;

public:
    CK_PROPERTY(_CurrentWorld);
};

// --------------------------------------------------------------------------------------------------------------------

