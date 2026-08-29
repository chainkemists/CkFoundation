#pragma once

#include <CoreMinimal.h>
#include <UObject/Object.h>

#include "Test_AssetValueDiff_Fixtures.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Shapes the write-back diff has to tell apart: a pod struct (one constructor literal), a struct
// carrying an object reference (one assignment per changed leaf, because AngelScript rejects the
// positional constructor there), and a class whose own constructor customises a struct field — the
// case a zero-init `InitializeStruct` comparand would report as a phantom edit.

USTRUCT()
struct FCkTest_WriteBack_PodLeaf
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    float _Scale = 1.0f;

    UPROPERTY(EditAnywhere)
    FName _Tag;
};

USTRUCT()
struct FCkTest_WriteBack_PodOnly
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    bool _Flag = false;

    UPROPERTY(EditAnywhere)
    int32 _Count = 0;
};

USTRUCT()
struct FCkTest_WriteBack_ObjectBearing
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32 _Depth = 0;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UObject> _Icon;

    UPROPERTY(EditAnywhere)
    FCkTest_WriteBack_PodLeaf _Leaf;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class UCkTest_WriteBack_Host : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    int32 _Count = 0;

    UPROPERTY(EditAnywhere)
    float _Scale = 1.0f;

    UPROPERTY(EditAnywhere)
    FString _Label;

    UPROPERTY(EditAnywhere)
    FName _Tag;

    UPROPERTY(EditAnywhere)
    FText _Caption;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UObject> _SoftRef;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UObject> _HardRef;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UObject> _ClassRef;

    UPROPERTY(EditAnywhere)
    TWeakObjectPtr<UObject> _WeakRef;

    UPROPERTY(EditAnywhere)
    TArray<int32> _Numbers;

    UPROPERTY(EditAnywhere)
    FCkTest_WriteBack_PodOnly _Pod;

    UPROPERTY(EditAnywhere)
    FCkTest_WriteBack_ObjectBearing _Bearing;

    UPROPERTY(Transient)
    int32 _Scratch = 0;

    // Not editable in the details panel, so it can never carry a user edit. Stands in for
    // `UCurveFloat::FloatCurve` (bare UPROPERTY) and `UCurveBase::AssetImportData`
    // (VisibleAnywhere), both of which report phantom differences on every real curve asset.
    UPROPERTY()
    int32 _NotEditable = 0;

    UPROPERTY(VisibleAnywhere)
    int32 _ReadOnly = 0;
};

// --------------------------------------------------------------------------------------------------------------------

// The constructor moves two struct fields off their struct-level defaults, so anything diffing
// against a fresh `InitializeStruct` buffer sees edits that the user never made.
UCLASS()
class UCkTest_WriteBack_CustomisedCdo : public UObject
{
    GENERATED_BODY()

public:
    UCkTest_WriteBack_CustomisedCdo();

    UPROPERTY(EditAnywhere)
    FCkTest_WriteBack_ObjectBearing _Bearing;

    UPROPERTY(EditAnywhere)
    FCkTest_WriteBack_PodOnly _Pod;

    // Non-null in the CDO, so clearing it in the details panel is a real override that must emit an
    // explicit `nullptr` rather than deleting the line.
    UPROPERTY(EditAnywhere)
    TObjectPtr<UObject> _Ref;
};

// --------------------------------------------------------------------------------------------------------------------
