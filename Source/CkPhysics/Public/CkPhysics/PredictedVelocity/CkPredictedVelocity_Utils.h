#pragma once

#include "CkPredictedVelocity_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkPredictedVelocity_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPHYSICS_API UCk_Utils_PredictedVelocity_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PredictedVelocity_UE);

public:
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_PredictedVelocity_ParamsData& InParams);

    static bool
    Has(
        FCk_Handle InHandle);

    static bool
    Ensure(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------