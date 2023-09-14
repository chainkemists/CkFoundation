#pragma once

#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_FragmentQuery_Policy : uint8
{
    CurrentEntity,
    EntityInRecord
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_FragmentQuery_Policy);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Lambda_InHandle,
    FCk_Handle, InHandle);


DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Predicate_InHandle_OutResult,
    FCk_Handle, InHandle,
    FCk_SharedBool, OutResult);


// --------------------------------------------------------------------------------------------------------------------