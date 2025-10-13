#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkService_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKHFSM_API FCk_Handle_Service : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Service);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Service);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Service_Command : uint8
{
    Start,
    Stop
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Service_Command);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Request_Service_Command : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Service_Command);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Service_Command);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Service_Command _Command = ECk_Service_Command::Start;

public:
    CK_PROPERTY_GET(_Command);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Service_Command, _Command);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Service,
    FCk_Handle_Service, InServiceHandle,
    FCk_Time, InDeltaT);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Service_MC,
    FCk_Handle_Service, InServiceHandle,
    FCk_Time, InDeltaT);

// --------------------------------------------------------------------------------------------------------------------