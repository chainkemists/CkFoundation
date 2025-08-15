#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "CkDynamic_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DestroyFilter : uint8
{
    None,
    IgnorePendingKill,
    Teardown
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDYNAMIC_API FCk_Fragment_DynamicFragment_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_DynamicFragment_Data);

private:
    UPROPERTY()
    FInstancedStruct _StructData;

public:
    CK_PROPERTY_GET(_StructData);
    CK_PROPERTY_GET_NON_CONST(_StructData);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_DynamicFragment_Data, _StructData);
};

// --------------------------------------------------------------------------------------------------------------------
