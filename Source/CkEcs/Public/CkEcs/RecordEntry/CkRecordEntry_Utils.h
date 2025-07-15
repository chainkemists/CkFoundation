#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecordEntry_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * RecordEntry is NOT exposed to Blueprints by design. See the RecordEntry Fragment for a more detailed reasoning.
 */
UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_RecordEntry_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RecordEntry_UE);

public:
    static auto
    Add(
        FCk_Handle& InHandle) -> void;

    [[nodiscard]]
    static auto
    Has(
        const FCk_Handle& InHandle) -> bool;

    [[nodiscard]]
    static auto
    Ensure(
        const FCk_Handle& InHandle) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
