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
              DisplayName="Get Bool Variable",
              Category = "Ck|Utils|Variables")
    static bool
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Bool Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Int32 Variable",
              Category = "Ck|Utils|Variables")
    static int32
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Int32 Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Int64 Variable",
              Category = "Ck|Utils|Variables")
    static int64
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Int64 Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Float Variable",
              Category = "Ck|Utils|Variables")
    static float
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Float Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Name Variable",
              Category = "Ck|Utils|Variables")
    static FName Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Name Variable",
              Category = "Ck|Utils|Variables")
    static void Set(
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
              DisplayName="Get String Variable",
              Category = "Ck|Utils|Variables")
    static const FString&
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set String Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Text Variable",
              Category = "Ck|Utils|Variables")
    static FText
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Text Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Vec3 Variable",
              Category = "Ck|Utils|Variables")
    static FVector
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Vec3 Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Vec2 Variable",
              Category = "Ck|Utils|Variables")
    static FVector2D
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Vec2 Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Rotator Variable",
              Category = "Ck|Utils|Variables")
    static FRotator
        Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Rotator Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Transform  Variable",
              Category = "Ck|Utils|Variables")
    static const FTransform&
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Transform Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get InstancedStruct Variable",
              Category = "Ck|Utils|Variables")
    static const FInstancedStruct&
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set InstancedStruct Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get Object Variable",
              Category = "Ck|Utils|Variables",
              meta = (DeterminesOutputType = "InObject"))
    static UObject*
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set Object Variable",
              Category = "Ck|Utils|Variables")
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
              DisplayName="Get SubclassOf Variable",
              Category = "Ck|Utils|Variables",
              meta = (DeterminesOutputType = "InObject"))
    static TSubclassOf<UObject>
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject);

    UFUNCTION(BlueprintCallable,
              DisplayName="Set SubclassOf Variable",
              Category = "Ck|Utils|Variables")
    static void
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InValue);
};

// --------------------------------------------------------------------------------------------------------------------
