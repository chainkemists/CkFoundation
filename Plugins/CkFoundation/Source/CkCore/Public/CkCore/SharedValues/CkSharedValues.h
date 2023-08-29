#pragma once

#include "CkMacros/CkMacros.h"

#include "CkSharedValues.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedBool
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedBool);

public:
    using Value_Type = bool;
    using Ptr_Type = TSharedPtr<Value_Type>;

public:
    FCk_SharedBool();
    explicit
    FCk_SharedBool(Value_Type InValue);

public:
    auto operator*() const -> Value_Type&;
    auto operator->() const -> Value_Type*;

private:
    Ptr_Type _Ptr;
};

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedBool_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedBool_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Core|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Get"))
    static bool
    Get(
        const FCk_SharedBool& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedBool& InShared,
        bool InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedBool",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedBool
    Make(
        bool InValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedInt
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedInt);

public:
    using Value_Type = int32;
    using Ptr_Type = TSharedPtr<Value_Type>;

public:
    FCk_SharedInt();
    explicit
    FCk_SharedInt(Value_Type InValue);

public:
    auto operator*() const -> Value_Type&;
    auto operator->() const -> Value_Type*;

private:
    Ptr_Type _Ptr;
};

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedInt_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedInt_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Core|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Get"))
    static int32
    Get(
        const FCk_SharedInt& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedInt& InShared,
        int32                           InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedInt",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedInt
    Make(
        int32 InValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedFloat
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedFloat);

public:
    using Value_Type = float;
    using Ptr_Type = TSharedPtr<Value_Type>;

public:
    FCk_SharedFloat();
    explicit
    FCk_SharedFloat(Value_Type InValue);

public:
    auto operator*() const -> Value_Type&;
    auto operator->() const -> Value_Type*;

private:
    Ptr_Type _Ptr;
};

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedFloat_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedFloat_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Core|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Get"))
    static float
    Get(
        const FCk_SharedFloat& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedFloat& InShared,
        float                             InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedFloat",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedFloat
    Make(
        float InValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedVector
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedVector);

public:
    using Value_Type = FVector;
    using Ptr_Type = TSharedPtr<Value_Type>;

public:
    FCk_SharedVector();
    explicit
    FCk_SharedVector(Value_Type InValue);

public:
    auto operator*() const -> Value_Type&;
    auto operator->() const -> Value_Type*;

private:
    Ptr_Type _Ptr;
};

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedVector_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedVector_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Core|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Get"))
    static FVector
    Get(
        const FCk_SharedVector& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedVector& InShared,
        FVector                            InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedVector",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedVector
    Make(
        FVector InValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedString
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedString);

public:
    using Value_Type = FString;
    using Ptr_Type = TSharedPtr<Value_Type>;

public:
    FCk_SharedString();
    explicit
    FCk_SharedString(Value_Type InValue);

public:
    auto operator*() const -> Value_Type&;
    auto operator->() const -> Value_Type*;

private:
    Ptr_Type _Ptr;
};

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedString_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedString_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Core|Utils|SharedString",
              meta     = (CompactNodeTitle = "Get"))
    static FString
    Get(
        const FCk_SharedString& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedString",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedString& InShared,
        FString                            InValue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedString",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedString
    Make(
        FString InValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedRotator
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedRotator);

public:
    using Value_Type = FRotator;
    using Ptr_Type = TSharedPtr<Value_Type>;

public:
    FCk_SharedRotator();
    explicit
    FCk_SharedRotator(Value_Type InValue);

public:
    auto operator*() const -> Value_Type&;
    auto operator->() const -> Value_Type*;

private:
    Ptr_Type _Ptr;
};

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_SharedRotator_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SharedRotator_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Core|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Get"))
    static FRotator
    Get(
        const FCk_SharedRotator& InShared);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Set"))
    static void
    Set(
        UPARAM(ref) FCk_SharedRotator& InShared,
        FRotator                            InValue);
public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Utils|SharedRotator",
              meta     = (CompactNodeTitle = "Make"))
    static FCk_SharedRotator
    Make(
        FRotator InValue);
};