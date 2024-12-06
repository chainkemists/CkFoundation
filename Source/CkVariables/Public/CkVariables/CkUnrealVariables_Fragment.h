#pragma once

#include "CkVariables/CkVariables_Fragment.h"

#include <GameplayTagContainer.h>
#include <InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKVARIABLES_API FFragment_Variable_Bool : public TFragment_Variables<bool> {};
    struct CKVARIABLES_API FFragment_Variable_Int32 : public TFragment_Variables<int32> {};
    struct CKVARIABLES_API FFragment_Variable_Int64 : public TFragment_Variables<int64> {};
    struct CKVARIABLES_API FFragment_Variable_Float : public TFragment_Variables<float> {};
    struct CKVARIABLES_API FFragment_Variable_Name : public TFragment_Variables<FName> {};
    struct CKVARIABLES_API FFragment_Variable_String : public TFragment_Variables<FString, const FString&> {};
    struct CKVARIABLES_API FFragment_Variable_Text : public TFragment_Variables<FText> {};
    struct CKVARIABLES_API FFragment_Variable_Vector : public TFragment_Variables<FVector> {};
    struct CKVARIABLES_API FFragment_Variable_Vector2D : public TFragment_Variables<FVector2D> {};
    struct CKVARIABLES_API FFragment_Variable_Rotator : public TFragment_Variables<FRotator> {};
    struct CKVARIABLES_API FFragment_Variable_Transform : public TFragment_Variables<FTransform, const FTransform&> {};
    struct CKVARIABLES_API FFragment_Variable_GameplayTag : public TFragment_Variables<FGameplayTag> {};
    struct CKVARIABLES_API FFragment_Variable_GameplayTagContainer : public TFragment_Variables<FGameplayTagContainer> {};

    struct CKVARIABLES_API FFragment_Variable_InstancedStruct : public TFragment_Variables<FInstancedStruct, const FInstancedStruct&> {};

    struct CKVARIABLES_API FFragment_Variable_UObject : public TFragment_Variables<TWeakObjectPtr<UObject>> {};
    struct CKVARIABLES_API FFragment_Variable_UObject_SubclassOf : public TFragment_Variables<TSubclassOf<UObject>> {};
    struct CKVARIABLES_API FFragment_Variable_Entity : public TFragment_Variables<FCk_Handle> {};
};

// --------------------------------------------------------------------------------------------------------------------