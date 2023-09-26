#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSharedValues.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedBool
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedBool);

public:
    using ValueType = bool;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedBool();
    explicit
    FCk_SharedBool(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedInt
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedInt);

public:
    using ValueType = int32;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedInt();
    explicit
    FCk_SharedInt(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedFloat
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedFloat);

public:
    using ValueType = float;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedFloat();
    explicit
    FCk_SharedFloat(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedVector
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedVector);

public:
    using ValueType = FVector;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedVector();
    explicit
    FCk_SharedVector(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedString
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedString);

public:
    using ValueType = FString;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedString();
    explicit
    FCk_SharedString(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedRotator
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedRotator);

public:
    using ValueType = FRotator;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedRotator();
    explicit
    FCk_SharedRotator(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------
