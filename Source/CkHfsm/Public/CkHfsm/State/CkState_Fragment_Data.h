#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkState_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKHFSM_API FCk_Handle_State : public FCk_Handle_TypeSafe 
{ 
    GENERATED_BODY() 
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_State); 
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_State);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_State_Command : uint8
{
    Enter,
    Exit,
    Evaluate
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_State_Command);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Request_State_Command : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_State_Command);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_State_Command);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_State_Command _Command = ECk_State_Command::Enter;

public:
    CK_PROPERTY_GET(_Command);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_State_Command, _Command);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Fragment_State_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_State_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, 
        meta = (AllowPrivateAccess = true, Categories = "HFSM.State"))
    FGameplayTag _Name;

public:
    CK_PROPERTY(_Name);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_State_ParamsData, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_State,
    FCk_Handle_State, InStateHandle,
    FCk_Time, InDeltaT);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_State_MC,
    FCk_Handle_State, InStateHandle,
    FCk_Time, InDeltaT);

// --------------------------------------------------------------------------------------------------------------------