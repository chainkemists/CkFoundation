#pragma once

#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkDelegates.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(NotBlueprintType)
struct FCk_DummyStructNeededForDelegatesCompilation
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Lambda_InHandle,
    FCk_Handle, InHandle);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Predicate_InHandle_OutResult,
    FCk_Handle, InHandle,
    FCk_SharedBool, OutResult);

// --------------------------------------------------------------------------------------------------------------------
