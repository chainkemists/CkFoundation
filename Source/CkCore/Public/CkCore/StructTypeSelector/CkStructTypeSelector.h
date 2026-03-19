#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkStructTypeSelector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_StructTypeSelector
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_StructTypeSelector);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UScriptStruct> _StructType;

public:
    auto Get_StructType() const -> UScriptStruct*;
    auto Set_StructType(UScriptStruct* InStructType) -> void;

    auto IsValid() const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
