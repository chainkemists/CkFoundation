#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkObjective_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKOBJECTIVE_API FCk_Handle_Objective : public FCk_Handle_TypeSafe{ GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Objective); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Objective);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ObjectiveStatus : uint8
{
    NotStarted,
    Active,
    Completed,
    Failed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ObjectiveStatus);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Objective_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Objective_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "Objective"))
    FGameplayTag _ObjectiveName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FText _DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, MultiLine = true))
    FText _Description;

public:
    CK_PROPERTY_GET(_ObjectiveName);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_Description);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Objective_ParamsData, _ObjectiveName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_Objective_Start : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Objective_Start);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Objective_Start);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_Objective_Complete : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Objective_Complete);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Objective_Complete);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _MetaData;

public:
    CK_PROPERTY(_MetaData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_Objective_Fail : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Objective_Fail);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Objective_Fail);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _MetaData;

public:
    CK_PROPERTY(_MetaData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_Objective_UpdateProgress : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Objective_UpdateProgress);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Objective_UpdateProgress);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _NewProgress = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _MetaData;

public:
    CK_PROPERTY_GET(_NewProgress);
    CK_PROPERTY(_MetaData);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Objective_UpdateProgress, _NewProgress);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_Objective_AddProgress : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Objective_AddProgress);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Objective_AddProgress);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _ProgressDelta = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _MetaData;

public:
    CK_PROPERTY_GET(_ProgressDelta);
    CK_PROPERTY(_MetaData);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Objective_AddProgress, _ProgressDelta);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Objective_StatusChanged,
    FCk_Handle_Objective, InObjective,
    ECk_ObjectiveStatus, InNewStatus);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Objective_StatusChanged_MC,
    FCk_Handle_Objective, InObjective,
    ECk_ObjectiveStatus, InNewStatus);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Delegate_Objective_ProgressChanged,
    FCk_Handle_Objective, InObjective,
    FGameplayTag, InMetaData,
    int32, InOldProgress,
    int32, InNewProgress);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FCk_Delegate_Objective_ProgressChanged_MC,
    FCk_Handle_Objective, InObjective,
    FGameplayTag, InMetaData,
    int32, InOldProgress,
    int32, InNewProgress);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Objective_Completed,
    FCk_Handle_Objective, InObjective,
    FGameplayTag, InMetaData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Objective_Completed_MC,
    FCk_Handle_Objective, InObjective,
    FGameplayTag, InMetaData);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Objective_Failed,
    FCk_Handle_Objective, InObjective,
    FGameplayTag, InMetaData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Objective_Failed_MC,
    FCk_Handle_Objective, InObjective,
    FGameplayTag, InMetaData);

// --------------------------------------------------------------------------------------------------------------------