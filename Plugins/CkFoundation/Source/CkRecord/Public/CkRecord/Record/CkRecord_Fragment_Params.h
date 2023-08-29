#pragma once

#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType) struct FCk_Record_Fragment_Params_ForceGen_Generated_File { GENERATED_BODY() };

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Lambda_InHandle,
    FCk_Handle, InHandle);


DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Predicate_InHandle_OutResult,
    FCk_Handle, InHandle,
    FCk_SharedBool, OutResult);


// --------------------------------------------------------------------------------------------------------------------