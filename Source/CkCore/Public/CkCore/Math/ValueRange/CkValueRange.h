#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkValueRange.generated.h"

// --------------------------------------------------------------------------------------------------------------------

#define DEFINE_CK_CORE_MATH_VALUE_RANGE_FUNCS(_type_)                                                                                  \
FORCEINLINE _type_ Get_ClampedValue     (_type_ InValue) const { return FMath::Clamp<_type_>(InValue, _Min, _Max); }                   \
FORCEINLINE bool Get_IsWithinExclusive  (_type_ InValue) const { return FMath::IsWithin(InValue, _Min, _Max); }                        \
FORCEINLINE bool Get_IsWithinExclusiveSq(_type_ InValue) const { return FMath::IsWithin(InValue, _Min * _Min, _Max * _Max); }          \
FORCEINLINE bool Get_IsWithinInclusive  (_type_ InValue) const { return FMath::IsWithinInclusive(InValue, _Min, _Max); }               \
FORCEINLINE bool Get_IsWithinInclusiveSq(_type_ InValue) const { return FMath::IsWithinInclusive(InValue, _Min * _Min, _Max * _Max); } \
FORCEINLINE _type_ Get_RandomValueInRange()              const { return FMath::RandRange(_Min, _Max); }                                \
FORCEINLINE FVector2D ToVector2D()                       const { return FVector2D(_Min, _Max); }

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_FloatRange
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FloatRange);

public:
    DEFINE_CK_CORE_MATH_VALUE_RANGE_FUNCS(float)

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Min = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Max = 0.0f;

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_FloatRange, _Min, _Max);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_FloatRange, [&]()
{
    return ck::Format(TEXT("Min: [{}], Max: [{}]"), InObj.Get_Min(), InObj.Get_Max());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_IntRange
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IntRange);

public:
    DEFINE_CK_CORE_MATH_VALUE_RANGE_FUNCS(int32)

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Min = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Max = 0.0f;

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_IntRange, _Min, _Max);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_IntRange, [&]()
{
    return ck::Format(TEXT("Min: [{}], Max: [{}]"), InObj.Get_Min(), InObj.Get_Max());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_FloatRange_0to1
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FloatRange_0to1);

public:
    FCk_FloatRange_0to1()  = default;
    explicit FCk_FloatRange_0to1(float InValue);

public:
    DEFINE_CK_CORE_MATH_VALUE_RANGE_FUNCS(float)

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float _Min = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float _Max = 1.0f;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
    float _Value = 0.0f;

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);
    CK_PROPERTY_GET(_Value);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_FloatRange_0to1, [&]()
{
    return ck::Format(TEXT("0 to 1: [{}]"), InObj.Get_Value());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_FloatRange_Minus1to1
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FloatRange_Minus1to1);

public:
    FCk_FloatRange_Minus1to1()  = default;
    explicit FCk_FloatRange_Minus1to1(float InValue);

public:
    DEFINE_CK_CORE_MATH_VALUE_RANGE_FUNCS(float)

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float _Min = -1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float _Max = 1.0f;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = "-1.0", UIMax = "1.0", ClampMin = "-1.0", ClampMax = "1.0"))
    float _Value = 0.0f;

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);
    CK_PROPERTY_GET(_Value);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_FloatRange_Minus1to1, [&]()
{
    return ck::Format(TEXT("-1 to +1: [{}]"), InObj.Get_Value());
});

// --------------------------------------------------------------------------------------------------------------------
