#pragma once

#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/Handle/CkHandle.h"

#include <InstancedStruct.h>

#include "CkDelegates.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(NotBlueprintType)
struct FCk_DummyStructNeededForDelegatesCompilation
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Lambda_InHandle,
    FCk_Handle, InHandle,
    FInstancedStruct, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Predicate_InHandle_OutResult,
    FCk_Handle, InHandle,
    FCk_SharedBool, OutResult,
    FInstancedStruct, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Predicate_In2Handles_OutResult,
    FCk_Handle, InA,
    FCk_Handle, InB,
    FCk_SharedBool, OutResult,
    FInstancedStruct, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Lambda_InActor,
    AActor*, InActor,
    FInstancedStruct, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Predicate_InActor_OutResult,
    AActor*, InActor,
    FCk_SharedBool, OutResult,
    FInstancedStruct, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_FourParams(
    FCk_Predicate_In2Actors_OutResult,
    AActor*, InA,
    AActor*, InB,
    FCk_SharedBool, OutResult,
    FInstancedStruct, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Lambda_InActorComponent,
    UActorComponent*, InActorComponent,
    FInstancedStruct, InOptionalPayload);

// --------------------------------------------------------------------------------------------------------------------
