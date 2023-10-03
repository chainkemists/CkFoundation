#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkNumericLimits.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_FloatNumericLimits
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FloatNumericLimits);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Min = std::numeric_limits<float>().lowest();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Max = std::numeric_limits<float>().max();

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_FloatNumericLimits, [&]()
{
    return ck::Format(TEXT("Min: [{}], Max: [{}]"), InObj.Get_Min(), InObj.Get_Max());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_DoubleNumericLimits
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DoubleNumericLimits);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Min = std::numeric_limits<double>().lowest();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Max = std::numeric_limits<double>().max();

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_DoubleNumericLimits, [&]()
{
    return ck::Format(TEXT("Min: [{}], Max: [{}]"), InObj.Get_Min(), InObj.Get_Max());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Int32NumericLimits
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Int32NumericLimits);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Min = std::numeric_limits<int32>().lowest();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Max = std::numeric_limits<int32>().max();

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_Int32NumericLimits, [&]()
{
    return ck::Format(TEXT("Min: [{}], Max: [{}]"), InObj.Get_Min(), InObj.Get_Max());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Int64NumericLimits
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Int64NumericLimits);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Min = std::numeric_limits<int64>().lowest();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _Max = std::numeric_limits<int64>().max();

public:
    CK_PROPERTY_GET(_Min);
    CK_PROPERTY_GET(_Max);
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_Int64NumericLimits, [&]()
{
    return ck::Format(TEXT("Min: [{}], Max: [{}]"), InObj.Get_Min(), InObj.Get_Max());
});

// --------------------------------------------------------------------------------------------------------------------
