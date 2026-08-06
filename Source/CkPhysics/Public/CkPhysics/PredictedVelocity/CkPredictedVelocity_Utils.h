#pragma once

#include "CkPredictedVelocity_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkPredictedVelocity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPHYSICS_API UCk_Utils_PredictedVelocity_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PredictedVelocity_UE);

public:
    static void
    Add(
        FCk_Handle& InHandle,
        const FCk_PredictedVelocity_Spec& InParams);

    static bool
    Has(
        const FCk_Handle& InHandle);

    static bool
    Ensure(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------