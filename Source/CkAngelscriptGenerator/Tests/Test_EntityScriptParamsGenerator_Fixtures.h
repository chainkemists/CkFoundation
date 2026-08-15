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

    UPROPERTY()
    bool Flag = false;

    UPROPERTY()
    FVector Offset = FVector::ZeroVector;

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;

    // The trap field: a nullptr UObject default is safe in field-decl position, not in a
    // positional-ctor argument (AS rejects the bare nullptr as `<null handle>`).
    UPROPERTY()
    TObjectPtr<USoundBase> Sound = nullptr;
};

USTRUCT()
struct FCkTest_ParamsGenerator_PodOnly
{
    GENERATED_BODY()

    UPROPERTY()
    bool Flag = false;

    UPROPERTY()
    FVector Offset = FVector::ZeroVector;
};

UCLASS()
class UCkTest_ParamsGenerator_Host : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY()
    FCkTest_ParamsGenerator_MixedFields Params;

    UPROPERTY()
    TObjectPtr<USoundBase> StrongSound;

    UPROPERTY()
    TWeakObjectPtr<USoundBase> WeakSound;

    UPROPERTY()
    TSoftObjectPtr<USoundBase> SoftSound;

    UPROPERTY()
    TSoftClassPtr<USoundBase> SoftSoundClass;

    UPROPERTY()
    TArray<TWeakObjectPtr<USoundBase>> WeakSounds;
};

UCLASS(NotBlueprintable, BlueprintType)
class UCkTest_ParamsGenerator_NativeEntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()
};

USTRUCT()
struct FCkTest_ParamsGenerator_WeakInjectionParams
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UObject> InjectedObject;
};

UCLASS(NotBlueprintable, BlueprintType)
class UCkTest_ParamsGenerator_WeakInjectionTarget : public UCk_EntityScript_UE
{
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UObject> InjectedObject;
};
