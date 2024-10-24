#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkEcsTemplate_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EcsTemplate_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECSTEMPLATE_API FCk_Handle_EcsTemplate : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EcsTemplate); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EcsTemplate);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSTEMPLATE_API FCk_Fragment_EcsTemplate_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EcsTemplate_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSTEMPLATE_API FCk_Fragment_MultipleEcsTemplate_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleEcsTemplate_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_EcsTemplate_ParamsData> _EcsTemplateParams;

public:
    CK_PROPERTY_GET(_EcsTemplateParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleEcsTemplate_ParamsData, _EcsTemplateParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSTEMPLATE_API FCk_Request_EcsTemplate_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_EcsTemplate_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_EcsTemplate_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------