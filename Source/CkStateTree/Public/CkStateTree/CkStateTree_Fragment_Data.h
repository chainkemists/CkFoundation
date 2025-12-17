#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkStateTree_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_StateTree_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSTATETREE_API FCk_Handle_StateTree : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_StateTree); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_StateTree);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Fragment_StateTree_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_StateTree_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Fragment_MultipleStateTree_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleStateTree_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_StateTree_ParamsData> _StateTreeParams;

public:
    CK_PROPERTY_GET(_StateTreeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleStateTree_ParamsData, _StateTreeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSTATETREE_API FCk_Request_StateTree_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_StateTree_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_StateTree_ExampleRequest);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_StateTree_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------