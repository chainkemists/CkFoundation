#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkLocator_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Locator_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSPATIALQUERY_API FCk_Handle_Locator : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Locator); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Locator);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_Locator_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Locator_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Fragment_MultipleLocator_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleLocator_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Locator_ParamsData> _LocatorParams;

public:
    CK_PROPERTY_GET(_LocatorParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleLocator_ParamsData, _LocatorParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSPATIALQUERY_API FCk_Request_Locator_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Locator_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Locator_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------