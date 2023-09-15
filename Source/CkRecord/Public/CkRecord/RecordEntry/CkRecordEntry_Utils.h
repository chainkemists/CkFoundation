#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecordEntry_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * RecordEntry is NOT exposed to Blueprints by design. See the RecordEntry Fragment for a more detailed reasoning.
 */
UCLASS(NotBlueprintable)
class CKRECORD_API UCk_Utils_RecordEntry_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RecordEntry_UE);

public:
    static auto
    Add(
        FCk_Handle InHandle) -> void;

    static auto
    Has(
        FCk_Handle InHandle) -> bool;

    static auto
    Ensure(
        FCk_Handle InHandle) -> bool;

private:

};

// --------------------------------------------------------------------------------------------------------------------
