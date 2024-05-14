#pragma once

#include "CkCore/Object/CkWorldContextObject.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <InstancedStruct.h>

#include "CkModularTrait_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ModularTrait_TargetOrInstigator : uint8
{
    Target,
    Instigator
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ModularTrait_TargetOrInstigator);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ModularTrait_TargetType : uint8
{
    FriendlyInstigator,
    FriendlyTarget,
    HostileInstigator,
    HostileTarget,
    ListenerIsInstigator,
    ListenerIsTarget
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ModularTrait_TargetType);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKMODULARTRAIT_API UCk_ModularTrait_Condition_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ModularTrait_Condition_UE);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    ECk_Condition_Result
    Evaluate(
        const FCk_Handle& InContext,
        const FInstancedStruct& InCustomParameters);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_ModularTrait_TriggerConditions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ModularTrait_TriggerConditions);

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced,
        meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UCk_ModularTrait_Condition_UE>> _Conditions;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKMODULARTRAIT_API UCk_ModularTrait_Config_UE : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ModularTrait_Config_UE);

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Trigger",
        meta = (AllowPrivateAccess = true, ShowOnlyInnerProperties = true))
    FCk_ModularTrait_TriggerConditions _TriggerCondition;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKMODULARTRAIT_API UCk_Utils_ModularTrait_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ModularTrait_UE);

public:

};

// --------------------------------------------------------------------------------------------------------------------
