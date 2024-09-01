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
              DisplayName="[Ck][Bool] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Bool")
    static bool
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Bool] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Bool")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        bool InValue);

    UFUNCTION(BlueprintPure,
        DisplayName="[Ck][Bool] Get Value (By Name)",
        Category = "Ck|Utils|Variables|Bool")
    static bool
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Bool] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Bool",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        bool InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Bool] Get All Variables",
              Category = "Ck|Utils|Variables|Bool",
              meta=(DevelopmentOnly))
    static const TMap<FName, bool>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Int32] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Int32")
    static int32
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Int32] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Int32")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        int32 InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Int32] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Int32")
    static int32
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Int32] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Int32",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        int32 InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Int32] Get All Variables",
              Category = "Ck|Utils|Variables|Int32",
              meta=(DevelopmentOnly))
    static const TMap<FName, int32>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Int64] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Int64")
    static int64
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Int64] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Int64")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        int64 InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Int64] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Int64")
    static int64
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Int64] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Int64",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        int64 InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Int64] Get All Variables",
              Category = "Ck|Utils|Variables|Int64",
              meta=(DevelopmentOnly))
    static const TMap<FName, int64>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Float] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Float")
    static float
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Float] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Float")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        float InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Float] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Float")
    static float
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Float] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Float",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        float InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Float] Get All Variables",
              Category = "Ck|Utils|Variables|Float",
              meta=(DevelopmentOnly))
    static const TMap<FName, float>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Name] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Name")
    static FName
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Name] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Name")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        FName InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Name] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Name")
    static FName
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Name] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Name",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        FName InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Name] Get All Variables",
              Category = "Ck|Utils|Variables|Name",
              meta=(DevelopmentOnly))
    static const TMap<FName, FName>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][String] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|String")
    static const FString&
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][String] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|String")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        const FString& InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][String] Get Value (By Name)",
              Category = "Ck|Utils|Variables|String")
    static const FString&
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][String] Set Value (By Name)",
              Category = "Ck|Utils|Variables|String",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        const FString& InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][String] Get All Variables",
              Category = "Ck|Utils|Variables|String",
              meta=(DevelopmentOnly))
    static const TMap<FName, FString>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Text] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Text")
    static FText
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Text] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Text")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        FText InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Text] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Text")
    static FText
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Text] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Text",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        FText InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Text] Get All Variables",
              Category = "Ck|Utils|Variables|Text",
              meta=(DevelopmentOnly))
    static const TMap<FName, FText>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Vec3] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Vec3")
    static FVector
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Vec3] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Vec3")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        FVector InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Vec3] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Vec3")
    static FVector
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Vec3] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Vec3",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        FVector InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Vec3] Get All Variables",
              Category = "Ck|Utils|Variables|Vec3",
              meta=(DevelopmentOnly))
    static const TMap<FName, FVector>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Vec2] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Vec2")
    static FVector2D
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Vec2] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Vec2")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        FVector2D InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Vec2] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Vec2")
    static FVector2D
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Vec2] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Vec2",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        FVector2D InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][FVector2D] Get All Variables",
              Category = "Ck|Utils|Variables|FVector2D",
              meta=(DevelopmentOnly))
    static const TMap<FName, FVector2D>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Rotator] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Rotator")
    static FRotator
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Rotator] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Rotator")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        FRotator InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Rotator] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Rotator")
    static FRotator
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Rotator] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Rotator",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        FRotator InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Rotator] Get All Variables",
              Category = "Ck|Utils|Variables|Rotator",
              meta=(DevelopmentOnly))
    static const TMap<FName, FRotator>&
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][Transform] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Transform")
    static const FTransform&
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Transform] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|Transform")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        const FTransform& InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Transform] Get Value (By Name)",
              Category = "Ck|Utils|Variables|Transform")
    static const FTransform&
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Transform] Set Value (By Name)",
              Category = "Ck|Utils|Variables|Transform",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        const FTransform& InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][Transform] Get All Variables",
              Category = "Ck|Utils|Variables|Transform",
              meta=(DevelopmentOnly))
    static const TMap<FName, FTransform>&
    Get_All(
        const FCk_Handle& InHandle);
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
    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][InstancedStruct] Get All Variables",
              Category = "Ck|Utils|Variables|InstancedStruct",
              meta=(DevelopmentOnly))
    static const TMap<FName, FInstancedStruct>&
    Get_All(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              CustomThunk,
              DisplayName="[Ck][InstancedStruct] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|InstancedStruct",
              meta=(CustomStructureParam = "InValue", BlueprintInternalUseOnly = true))
    static void
    INTERNAL__Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        const int32& InValue);
    DECLARE_FUNCTION(execINTERNAL__Set);

    static void
    Set(
        FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        const FInstancedStruct& InValue);

public:
    UFUNCTION(BlueprintCallable,
              CustomThunk,
              DisplayName="[Ck][InstancedStruct] Set Value (By Name)",
              Category = "Ck|Utils|Variables|InstancedStruct",
              meta=(CustomStructureParam = "InValue", BlueprintInternalUseOnly = true))
    static void
    INTERNAL__Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        const int32& InValue);
    DECLARE_FUNCTION(execINTERNAL__Set_ByName);

    static void
    Set_ByName(
        FCk_Handle& InHandle,
        FName InVariableName,
        const FInstancedStruct& InValue);

public:
    UFUNCTION(BlueprintPure,
              CustomThunk,
              Displayname = "[Ck][InstancedStruct] Get Value (By Name)",
              Category = "Ck|Utils|Variables|InstancedStruct",
              meta=(CustomStructureParam = "OutValue", BlueprintInternalUseOnly = true))
    static void
    INTERNAL__Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail,
        int32& OutValue);
    DECLARE_FUNCTION(execINTERNAL__Get_ByName);

    static const FInstancedStruct&
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        ECk_SucceededFailed& OutSuccessFail);

public:
    UFUNCTION(BlueprintPure,
              CustomThunk,
              Displayname = "[Ck][InstancedStruct] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|InstancedStruct",
              meta=(CustomStructureParam = "OutValue", BlueprintInternalUseOnly = true))
    static void
    INTERNAL__Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail,
        int32& OutValue);
    DECLARE_FUNCTION(execINTERNAL__Get);

    static const FInstancedStruct&
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_SucceededFailed& OutSuccessFail);
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
              DisplayName="[Ck][UObject] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|UObject",
              meta = (DeterminesOutputType = "InObject"))
    static UObject*
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][UObject] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|UObject")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        UObject* InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][UObject] Get Value (By Name)",
              Category = "Ck|Utils|Variables|UObject",
              meta = (DeterminesOutputType = "InObject", DeprecatedFunction))
    static UObject*
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][UObject] Set Value (By Name)",
              Category = "Ck|Utils|Variables|UObject",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        UObject* InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][UObject] Get All Variables",
              Category = "Ck|Utils|Variables|UObject",
              meta=(DevelopmentOnly))
    static TMap<FName, UObject*>
    Get_All(
        const FCk_Handle& InHandle);
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
              DisplayName="[Ck][SubclassOf] Get Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|SubclassOf",
              meta = (DeterminesOutputType = "InObject"))
    static TSubclassOf<UObject>
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][SubclassOf] Set Value (By GameplayTag)",
              Category = "Ck|Utils|Variables|SubclassOf")
    static void
    Set(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InValue);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][SubclassOf] Get Value (By Name)",
              Category = "Ck|Utils|Variables|SubclassOf",
              meta = (DeterminesOutputType = "InObject", DeprecatedFunction))
    static TSubclassOf<UObject>
    Get_ByName(
        const FCk_Handle&    InHandle,
        FName                InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_SucceededFailed& OutSuccessFail);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][SubclassOf] Set Value (By Name)",
              Category = "Ck|Utils|Variables|SubclassOf",
              meta=(DeprecatedFunction))
    static void
    Set_ByName(
        UPARAM(ref) FCk_Handle& InHandle,
        FName InVariableName,
        TSubclassOf<UObject> InValue);

    UFUNCTION(BlueprintCallable,
              DisplayName="[Ck][SubclassOf] Get All Variables",
              Category = "Ck|Utils|Variables|SubclassOf",
              meta=(DevelopmentOnly))
    static const TMap<FName, TSubclassOf<UObject>>&
    Get_All(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
