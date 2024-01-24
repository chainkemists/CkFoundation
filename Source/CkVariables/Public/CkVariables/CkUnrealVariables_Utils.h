#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkVariables/CkUnrealVariables_Fragment.h"

#include <GameplayTagContainer.h>

#include "CkUnrealVariables_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Bool_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Bool_UE);

public:
    using FragmentType = ck::FFragment_Variable_Bool;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[BoolVariable] Get Value",
              Category = "Ck|Utils|Variables|Bool")
    static bool
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[BoolVariable] Set Value",
              Category = "Ck|Utils|Variables|Bool")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        bool InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Int32_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Int32_UE);

public:
    using FragmentType = ck::FFragment_Variable_Int32;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Int32Variable] Get Value",
              Category = "Ck|Utils|Variables|Int32")
    static int32
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Int32Variable] Set Value",
              Category = "Ck|Utils|Variables|Int32")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        int32 InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Int64_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Int64_UE);

public:
    using FragmentType = ck::FFragment_Variable_Int64;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Int64Variable] Get Value",
              Category = "Ck|Utils|Variables|Int64")
    static int64
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Int64Variable] Set Value",
              Category = "Ck|Utils|Variables|Int64")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        int64 InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Float_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Float_UE);

public:
    using FragmentType = ck::FFragment_Variable_Float;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[FloatVariable] Get Value",
              Category = "Ck|Utils|Variables|Float")
    static float
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[FloatVariable] Set Value",
              Category = "Ck|Utils|Variables|Float")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        float InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Name_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Name_UE);

public:
    using FragmentType = ck::FFragment_Variable_Name;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[NameVariable] Get Value",
              Category = "Ck|Utils|Variables|Name")
    static FName
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[NameVariable] Set Value",
              Category = "Ck|Utils|Variables|Name")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        FName InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_String_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_String_UE);

public:
    using FragmentType = ck::FFragment_Variable_String;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[StringVariable] Get Value",
              Category = "Ck|Utils|Variables|String")
    static const FString&
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[StringVariable] Set Value",
              Category = "Ck|Utils|Variables|String")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        const FString& InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Text_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Text_UE);

public:
    using FragmentType = ck::FFragment_Variable_Text;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[TextVariable] Get Value",
              Category = "Ck|Utils|Variables|Text")
    static FText
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[TextVariable] Set Value",
              Category = "Ck|Utils|Variables|Text")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        FText InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Vector_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Vector_UE);

public:
    using FragmentType = ck::FFragment_Variable_Vector;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Vec3Variable] Get Value",
              Category = "Ck|Utils|Variables|Vec3")
    static FVector
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Vec3Variable] Set Value",
              Category = "Ck|Utils|Variables|Vec3")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        FVector InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Vector2D_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Vector2D_UE);

public:
    using FragmentType = ck::FFragment_Variable_Vector2D;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Vec2Variable] Get Value",
              Category = "Ck|Utils|Variables|Vec2")
    static FVector2D
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Vec2Variable] Set Value",
              Category = "Ck|Utils|Variables|Vec2")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        FVector2D InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Rotator_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Rotator_UE);

public:
    using FragmentType = ck::FFragment_Variable_Rotator;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[RotatorVariable] Get Value",
              Category = "Ck|Utils|Variables|Rotator")
    static FRotator
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[RotatorVariable] Set Value",
              Category = "Ck|Utils|Variables|Rotator")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        FRotator InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_Transform_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_Transform_UE);

public:
    using FragmentType = ck::FFragment_Variable_Transform;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[TransformVariable] Get Value",
              Category = "Ck|Utils|Variables|Transform")
    static const FTransform&
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[TransformVariable] Set Value",
              Category = "Ck|Utils|Variables|Transform")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        const FTransform& InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_InstancedStruct_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_InstancedStruct_UE);

public:
    using FragmentType = ck::FFragment_Variable_InstancedStruct;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[InstancedStructVariable] Get Value",
              Category = "Ck|Utils|Variables|InstancedStruct")
    static const FInstancedStruct&
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="[InstancedStructVariable] Set Value",
              Category = "Ck|Utils|Variables|InstancedStruct")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        const FInstancedStruct& InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_UObject_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_UObject_UE);

public:
    using FragmentType = ck::FFragment_Variable_UObject;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[UObjectVariable] Get Value",
              Category = "Ck|Utils|Variables|UObject",
              meta = (DeterminesOutputType = "InObject"))
    static UObject*
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject);

    UFUNCTION(BlueprintCallable,
              DisplayName="[UObjectVariable] Set Value",
              Category = "Ck|Utils|Variables|UObject")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        UObject* InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKVARIABLES_API UCk_Utils_Variables_UObject_SubclassOf_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Variables_UObject_SubclassOf_UE);

public:
    using FragmentType = ck::FFragment_Variable_UObject_SubclassOf;
    using UtilsType = ck::TUtils_Variables<FragmentType>;

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[SubclassOfVariable] Get Value",
              Category = "Ck|Utils|Variables|SubclassOf",
              meta = (DeterminesOutputType = "InObject"))
    static TSubclassOf<UObject>
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject);

    UFUNCTION(BlueprintCallable,
              DisplayName="[SubclassOfVariable] Set Value",
              Category = "Ck|Utils|Variables|SubclassOf")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InValue);
};

// --------------------------------------------------------------------------------------------------------------------
