#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <InstancedStruct.h>

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
struct CKCORE_API FCk_SharedVec3
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedVec3);

public:
    using ValueType = FVector;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedVec3();
    explicit
    FCk_SharedVec3(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedVec2
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedVec2);

public:
    using ValueType = FVector2D;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedVec2();
    explicit
    FCk_SharedVec2(ValueType InValue);

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
struct CKCORE_API FCk_SharedName
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedName);

public:
    using ValueType = FName;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedName();
    explicit
    FCk_SharedName(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedText
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedText);

public:
    using ValueType = FText;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedText();
    explicit
    FCk_SharedText(ValueType InValue);

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

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedTransform
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedTransform);

public:
    using ValueType = FTransform;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedTransform();
    explicit
    FCk_SharedTransform(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_SharedInstancedStruct
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SharedInstancedStruct);

public:
    using ValueType = FInstancedStruct;
    using PtrType = TSharedPtr<ValueType, ESPMode::NotThreadSafe>;

public:
    FCk_SharedInstancedStruct();
    explicit
    FCk_SharedInstancedStruct(ValueType InValue);

public:
    auto operator*() const -> ValueType&;
    auto operator->() const -> ValueType*;

private:
    PtrType _Ptr;
};

// --------------------------------------------------------------------------------------------------------------------
