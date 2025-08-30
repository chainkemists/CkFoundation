#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkObjective/Objective/CkObjective_Fragment_Data.h"
#include "CkObjective/Objective/CkObjective_EntityScript.h"

#include <GameplayTagContainer.h>

#include "CkObjectiveOwner_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKOBJECTIVE_API FCk_Handle_ObjectiveOwner : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ObjectiveOwner); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ObjectiveOwner);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_ObjectiveOwner_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectiveOwner_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TArray<TSubclassOf<UCk_Objective_EntityScript>> _DefaultObjectives;

public:
    CK_PROPERTY_GET(_DefaultObjectives);

    CK_DEFINE_CONSTRUCTORS(FCk_ObjectiveOwner_ParamsData, _DefaultObjectives);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_ObjectiveOwner_AddObjective : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ObjectiveOwner_AddObjective);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ObjectiveOwner_AddObjective);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSubclassOf<UCk_Objective_EntityScript> _ObjectiveClass;

public:
    CK_PROPERTY_GET(_ObjectiveClass);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ObjectiveOwner_AddObjective, _ObjectiveClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKOBJECTIVE_API FCk_Request_ObjectiveOwner_RemoveObjective : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ObjectiveOwner_RemoveObjective);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ObjectiveOwner_RemoveObjective);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Objective _ObjectiveHandle;

public:
    CK_PROPERTY_GET(_ObjectiveHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ObjectiveOwner_RemoveObjective, _ObjectiveHandle);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ObjectiveOwner_ObjectiveAdded,
    FCk_Handle_ObjectiveOwner, InOwner,
    FCk_Handle_Objective, InObjective);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ObjectiveOwner_ObjectiveAdded_MC,
    FCk_Handle_ObjectiveOwner, InOwner,
    FCk_Handle_Objective, InObjective);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ObjectiveOwner_ObjectiveRemoved,
    FCk_Handle_ObjectiveOwner, InOwner,
    FCk_Handle_Objective, InObjective);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ObjectiveOwner_ObjectiveRemoved_MC,
    FCk_Handle_ObjectiveOwner, InOwner,
    FCk_Handle_Objective, InObjective);

// --------------------------------------------------------------------------------------------------------------------