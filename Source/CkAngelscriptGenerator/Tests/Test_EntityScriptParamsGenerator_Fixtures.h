// Test fixtures for `CkAngelscriptGenerator.UnitTests.ParamsGenerator.*`. The USTRUCT below
// mirrors the failure shape that originally triggered the OpenSign codegen bug: a struct with
// a mix of POD fields (overridable on a subclass CDO) and a UObject* field defaulting to nullptr.
// The companion UCLASS exists so we can read a properly-reflected CDO + FStructProperty through
// the normal reflection path.

#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include <CoreMinimal.h>
#include <Sound/SoundBase.h>
#include <UObject/Object.h>

#include "Test_EntityScriptParamsGenerator_Fixtures.generated.h"

USTRUCT()
struct FCkTest_ParamsGenerator_MixedFields
{
    GENERATED_BODY()

    UPROPERTY() bool        Flag = false;
    UPROPERTY() FVector     Offset = FVector::ZeroVector;
    UPROPERTY() FRotator    Rotation = FRotator::ZeroRotator;

    // The trap field — exercising the bug class. Defaults to nullptr in the struct's own
    // initializer; the generator emits this safely as `= nullptr` in field-decl position.
    UPROPERTY() TObjectPtr<USoundBase> Sound = nullptr;
};

USTRUCT()
struct FCkTest_ParamsGenerator_PodOnly
{
    GENERATED_BODY()

    UPROPERTY() bool    Flag = false;
    UPROPERTY() FVector Offset = FVector::ZeroVector;
};

UCLASS()
class UCkTest_ParamsGenerator_Host : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY() FCkTest_ParamsGenerator_MixedFields Params;

    UPROPERTY() TObjectPtr<USoundBase> StrongSound;
    UPROPERTY() TWeakObjectPtr<USoundBase> WeakSound;
    UPROPERTY() TSoftObjectPtr<USoundBase> SoftSound;
    UPROPERTY() TSoftClassPtr<USoundBase> SoftSoundClass;
    UPROPERTY() TArray<TWeakObjectPtr<USoundBase>> WeakSounds;
};

// Concrete native entity-script subclass — the "included" shape for the class-filter test
// (the UCk_EntityScript_UE base itself is UCLASS(Abstract) and therefore excluded).
UCLASS(NotBlueprintable, BlueprintType)
class UCkTest_ParamsGenerator_NativeEntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()
};

USTRUCT()
struct FCkTest_ParamsGenerator_WeakInjectionParams
{
    GENERATED_BODY()

    UPROPERTY() TWeakObjectPtr<UObject> InjectedObject;
};

UCLASS(NotBlueprintable, BlueprintType)
class UCkTest_ParamsGenerator_WeakInjectionTarget : public UCk_EntityScript_UE
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UObject> InjectedObject;
};
