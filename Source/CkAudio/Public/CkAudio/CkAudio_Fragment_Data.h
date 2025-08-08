#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkAudio_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Audio_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAUDIO_API FCk_Handle_Audio : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Audio); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Audio);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Fragment_Audio_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Audio_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Fragment_MultipleAudio_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleAudio_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Audio_ParamsData> _AudioParams;

public:
    CK_PROPERTY_GET(_AudioParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleAudio_ParamsData, _AudioParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKAUDIO_API FCk_Request_Audio_ExampleRequest : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_Audio_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_Audio_ExampleRequest);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Audio_ExampleRequest);
};

// --------------------------------------------------------------------------------------------------------------------