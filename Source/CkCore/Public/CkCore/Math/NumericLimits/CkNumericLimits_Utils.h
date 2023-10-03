#pragma once
#include "CkCore/Macros/CkMacros.h"

#include "CkNumericLimits_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_FloatNumericLimits_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatNumericLimits_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Float",
              DisplayName = "Get Is Float Limit Max")
    static bool
    Get_IsNumericLimitMax(float InValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Float",
              DisplayName = "Get Is Float Limit Min")
    static bool
    Get_IsNumericLimitMin(float InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_DoubleNumericLimits_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DoubleNumericLimits_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Double",
              DisplayName = "Get Is Double Limit Max")
    static bool
    Get_IsNumericLimitMax(double InValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Double",
              DisplayName = "Get Is Double Limit Min")
    static bool
    Get_IsNumericLimitMin(double InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Int32NumericLimits_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Int32NumericLimits_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Int32",
              DisplayName = "Get Is Int32 Limit Max")
    static bool
    Get_IsNumericLimitMax(int32 InValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Int32",
              DisplayName = "Get Is Int32 Limit Min")
    static bool
    Get_IsNumericLimitMin(int32 InValue);
};


// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Int64NumericLimits_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Int64NumericLimits_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Int64",
              DisplayName = "Get Is Int64 Limit Max")
    static bool
    Get_IsNumericLimitMax(int64 InValue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Math|NumericLimits|Int64",
              DisplayName = "Get Is Int64 Limit Min")
    static bool
    Get_IsNumericLimitMin(int64 InValue);
};

// --------------------------------------------------------------------------------------------------------------------
