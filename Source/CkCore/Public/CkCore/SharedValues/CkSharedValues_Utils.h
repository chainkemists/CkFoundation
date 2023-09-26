#pragma once

#include "CkSharedValues.h"

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkSharedValues_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedBool_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedBool_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Get"))
    static bool
    Get(
        const FCk_SharedBool& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedBool& InShared,
        bool InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedBool
    Make(
        bool InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedInt_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedInt_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Get"))
    static int32
    Get(
        const FCk_SharedInt& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedInt& InShared,
        int32                           InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedInt
    Make(
        int32 InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedFloat_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedFloat_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Get"))
    static float
    Get(
        const FCk_SharedFloat& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedFloat& InShared,
        float InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedFloat
    Make(
        float InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedVector_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedVector_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Get"))
    static FVector
    Get(
        const FCk_SharedVector& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedVector& InShared,
        FVector InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedVector
    Make(
        FVector InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedString_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedString_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SharedString",
              meta     = (CompactNodeTitle = "Get"))
    static FString
    Get(
        const FCk_SharedString& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedString",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedString& InShared,
        FString InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedString",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedString
    Make(
        FString InValue);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedRotator_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedRotator_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Get"))
    static FRotator
    Get(
        const FCk_SharedRotator& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedRotator& InShared,
        FRotator InValue);
public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedRotator
    Make(
        FRotator InValue);
};

// --------------------------------------------------------------------------------------------------------------------
