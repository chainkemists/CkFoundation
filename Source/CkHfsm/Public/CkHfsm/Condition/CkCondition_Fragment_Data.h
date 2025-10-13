#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkCondition_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKHFSM_API FCk_Handle_Condition : public FCk_Handle_TypeSafe 
{ 
    GENERATED_BODY() 
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Condition); 
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Condition);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Condition_Command : uint8
{
    StartOrResumeEvaluating,
    PauseEvaluation,
    StopEvaluating
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Condition_Command);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Condition_Result : uint8
{
    Undetermined,
    Pass,
    Fail
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Condition_Result);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Condition_MarkResult : uint8
{
    Passed,
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Condition_MarkResult);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Request_Condition_Command : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Condition_Command);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Condition_Command);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Condition_Command _Command = ECk_Condition_Command::StartOrResumeEvaluating;

public:
    CK_PROPERTY_GET(_Command);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Condition_Command, _Command);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Request_Condition_MarkResult : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Condition_MarkResult);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Condition_MarkResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Condition_MarkResult _Result = ECk_Condition_MarkResult::Passed;

public:
    CK_PROPERTY_GET(_Result);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Condition_MarkResult, _Result);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKHFSM_API FCk_Fragment_Condition_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Condition_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _NegateResult = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _IsEventDriven = false;

public:
    CK_PROPERTY_GET(_NegateResult);
    CK_PROPERTY_GET(_IsEventDriven);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Condition_ParamsData, _NegateResult, _IsEventDriven);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Condition,
    FCk_Handle_Condition, InConditionHandle,
    FCk_Time, InDeltaT);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Condition_MC,
    FCk_Handle_Condition, InConditionHandle,
    FCk_Time, InDeltaT);

// --------------------------------------------------------------------------------------------------------------------